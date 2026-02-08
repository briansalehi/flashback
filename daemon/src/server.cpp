#include <algorithm>
#include <chrono>
#include <flashback/server.hpp>
#include <flashback/exception.hpp>
#include <format>
#include <iostream>
#include <sodium.h>
#include <sstream>

using namespace flashback;

server::server(std::shared_ptr<basic_database> database)
    : m_database{database}
{
    if (sodium_init() < 0)
    {
        throw std::runtime_error("Server: sodium library cannot be initialized");
    }
}

grpc::Status server::SignIn(grpc::ServerContext* context, const SignInRequest* request, SignInResponse* response)
{
    try
    {
        if (!request->has_user() || request->user().email().empty() || request->user().password().empty() || request->user().device().empty())
        {
            throw client_exception("incomplete credentials");
        }

        if (!m_database->user_exists(request->user().email()))
        {
            throw client_exception(std::format("user is not registered with email {}", request->user().email()));
        }

        auto user{m_database->get_user(request->user().email())};
        user->set_token(generate_token());
        user->set_device(request->user().device());

        if (!password_is_valid(user->hash(), request->user().password()))
        {
            throw client_exception("invalid credentials");
        }

        if (!m_database->create_session(user->id(), user->token(), user->device()))
        {
            throw client_exception(std::format("cannot create session for user {}", user->id()));
        }

        std::cerr << std::format("Client {}: signed in with device {}\n", user->id(), user->device());

        user->clear_id();
        user->clear_hash();
        user->clear_password();
        response->set_success(true);
        response->set_details("sign in successful");
        response->set_allocated_user(user.release());
    }
    catch (client_exception const& exp)
    {
        response->set_success(false);
        response->set_details(exp.code());
        response->clear_user();
        std::cerr << std::format("Client: {}\n", exp.what());
    }
    catch (std::exception const& exp)
    {
        response->set_success(false);
        response->set_details("internal error");
        response->clear_user();
        std::cerr << std::format("Server: {}\n", exp.what());
    }

    return grpc::Status::OK;
}

grpc::Status server::SignUp(grpc::ServerContext* context, const SignUpRequest* request, SignUpResponse* response)
{
    try
    {
        if (!request->has_user() || request->user().name().empty() || request->user().email().empty() || request->user().device().empty() || request->user().password().empty())
        {
            throw client_exception("incomplete credentials");
        }

        auto user{std::make_unique<User>(request->user())};

        if (m_database->user_exists(request->user().email()))
        {
            throw client_exception(std::format("user {} is already registered", request->user().email()));
            // auto existing_user{m_database->get_user(request->user().email())};
        }

        user->set_hash(calculate_hash(user->password()));
        uint64_t user_id{m_database->create_user(user->name(), user->email(), user->hash())};
        user->clear_password();
        user->clear_hash();
        user->clear_id();

        if (user_id > 0)
        {
            response->set_success(true);
            response->set_details("signup successful");
            response->set_allocated_user(user.release());
        }
        else
        {
            response->set_success(false);
            response->set_details("signup failed");
        }
    }
    catch (pqxx::unique_violation const& exp)
    {
        response->set_success(false);
        response->set_details("user already exists");
        std::cerr << std::format("Client: {}\n", exp.what());
    }
    catch (client_exception const& exp)
    {
        response->set_success(false);
        response->set_details(exp.code());
        std::cerr << std::format("Client: {}\n", exp.what());
    }
    catch (std::exception const& exp)
    {
        response->set_success(false);
        response->set_details("internal error");
        std::cerr << std::format("Server: {}\n", exp.what());
    }

    return grpc::Status::OK;
}

grpc::Status server::ResetPassword(grpc::ServerContext* context, ResetPasswordRequest const* request, ResetPasswordResponse* response)
{
    try
    {
        if (request->has_user() && (request->user().id() == 0 || request->user().email().empty() || request->user().password().empty() || request->user().device().empty()))
        {
            throw client_exception("incomplete credentials");
        }

        std::string hash{calculate_hash(request->user().password())};

        auto user{std::make_unique<User>(request->user())};
        user->clear_password();
        user->clear_device();

        m_database->reset_password(user->id(), hash);

        response->set_allocated_user(user.release());
    }
    catch (std::exception const& exp)
    {
        std::cerr << exp.what() << std::endl;
    }

    return grpc::Status::OK;
}

grpc::Status server::EditUser(grpc::ServerContext* context, EditUserRequest const* request, EditUserResponse* response)
{
    // name, email
    return grpc::Status::OK;
}

grpc::Status server::VerifySession(grpc::ServerContext* context, VerifySessionRequest const* request, VerifySessionResponse* response)
{
    try
    {
        response->set_valid(request->has_user() && session_is_valid(request->user()));
    }
    catch (std::exception const& exp)
    {
        response->set_valid(false);
    }

    return grpc::Status::OK;
}

grpc::Status server::CreateRoadmap(grpc::ServerContext* context, CreateRoadmapRequest const* request, CreateRoadmapResponse* response)
{
    response->clear_roadmap();

    try
    {
        if (request->has_user() && session_is_valid(request->user()))
        {
            auto roadmap{std::make_unique<Roadmap>(m_database->create_roadmap(request->user().id(), request->name()))};
            response->set_allocated_roadmap(roadmap.release());
        }
    }
    catch (client_exception const& exp)
    {
        std::cerr << std::format("Client: {}", exp.what());
    }
    catch (pqxx::unique_violation const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return grpc::Status::OK;
}

grpc::Status server::GetRoadmaps(grpc::ServerContext* context, GetRoadmapsRequest const* request, GetRoadmapsResponse* response)
{
    try
    {
        std::shared_ptr<User> const user{m_database->get_user(request->user().token(), request->user().device())};

        if (!request->has_user() || user == nullptr)
        {
            throw client_exception("unauthorized request for roadmaps");
        }

        std::vector<Roadmap> const roadmaps{m_database->get_roadmaps(user->id())};
        std::clog << std::format("Client {}: collected {} roadmaps\n", user->id(), roadmaps.size());

        for (Roadmap const& roadmap: roadmaps)
        {
            auto allocated_roadmap = response->add_roadmap();
            allocated_roadmap->set_id(roadmap.id());
            allocated_roadmap->set_name(roadmap.name());
        }
    }
    catch (client_exception const& exp)
    {
        std::cerr << std::format("Client {}: {}\n", request->user().token(), exp.what());
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: failed to collect roadmaps for client {} because:\n{}\n", request->user().token(), exp.what());
    }

    return grpc::Status::OK;
}

grpc::Status server::GetStudyResources(grpc::ServerContext* context, GetStudyResourcesRequest const* request, GetStudyResourcesResponse* response)
{
    return grpc::Status::OK;
}

grpc::Status server::RenameRoadmap(grpc::ServerContext* context, RenameRoadmapRequest const* request, RenameRoadmapResponse* response)
{
    try
    {
        if (request->has_user() && request->has_roadmap() && session_is_valid(request->user()))
        {
            m_database->rename_roadmap(request->roadmap().id(), request->roadmap().name());
        }
    }
    catch (flashback::client_exception const& exp)
    {
        std::cerr << std::format("Client: {}", exp.what());
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return grpc::Status::OK;
}

grpc::Status server::RemoveRoadmap(grpc::ServerContext* context, RemoveRoadmapRequest const* request, RemoveRoadmapResponse* response)
{
    try
    {
        if (request->has_user() && request->has_roadmap() && session_is_valid(request->user()))
        {
            m_database->remove_roadmap(request->user().id());
        }
    }
    catch (flashback::client_exception const& exp)
    {
        std::cerr << std::format("Client: {}", exp.what());
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return grpc::Status::OK;
}

grpc::Status server::SearchRoadmaps(grpc::ServerContext* context, SearchRoadmapsRequest const* request, SearchRoadmapsResponse* response)
{
    try
    {
        if (request->has_user() && session_is_valid(request->user()))
        {
            for (auto const& [similarity, matched]: m_database->search_roadmaps(request->token()))
            {
                flashback::Roadmap* roadmap = response->add_roadmap();
                roadmap->set_id(matched.id());
                roadmap->set_name(matched.name());
            }
        }
    }
    catch (flashback::client_exception const& exp)
    {
        std::cerr << std::format("Client: {}", exp.what());
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return grpc::Status::OK;
}

grpc::Status server::CloneRoadmap(grpc::ServerContext* context, CloneRoadmapRequest const* request, CloneRoadmapResponse* response)
{
    try
    {
        response->set_success(false);
        response->clear_code();
        response->clear_details();

        if (request->has_user() && session_is_valid(request->user()))
        {
            if (request->has_roadmap())
            {
                Roadmap roadmap{m_database->clone_roadmap(request->user().id(), request->roadmap().id())};
                response->set_allocated_roadmap(std::make_unique<Roadmap>(roadmap).release());
                response->set_success(true);
            }
            else
            {
                response->set_code(4);
                response->set_details("invalid roadmap");
            }
        }
        else
        {
            response->set_code(3);
            response->set_details("invalid user");
        }
    }
    catch (flashback::client_exception const& exp)
    {
        response->set_code(2);
        response->set_details(exp.what());
        response->set_success(false);
        std::cerr << std::format("Client: {}", exp.what());
    }
    catch (std::exception const& exp)
    {
        response->set_code(1);
        response->set_details("internal error");
        response->set_success(false);
        std::cerr << std::format("Server: {}", exp.what());
    }

    return grpc::Status::OK;
}

grpc::Status server::GetMilestones(grpc::ServerContext* context, GetMilestonesRequest const* request, GetMilestonesResponse* response)
{
    response->clear_milestones();
    response->set_success(false);
    response->set_code(0);
    response->clear_details();

    try
    {
        if (request->has_user() && session_is_valid(request->user()))
        {
            for (Milestone const& m: m_database->get_milestones(request->roadmap_id()))
            {
                Milestone* milestone{response->add_milestones()};
                milestone->set_id(m.id());
                milestone->set_position(m.position());
                milestone->set_name(m.name());
                milestone->set_level(m.level());
            }
            response->set_success(true);
        }
    }
    catch (client_exception const& exp)
    {
        response->set_details(exp.what());
        response->set_code(1);
    }
    catch (std::exception const& exp)
    {
        response->set_details("internal error");
        response->set_code(1);
        std::cerr << std::format("Server: {}", exp.what());
    }
    return grpc::Status::OK;
}

grpc::Status server::AddMilestone(grpc::ServerContext* context, AddMilestoneRequest const* request, AddMilestoneResponse* response)
{
    try
    {
        response->set_success(false);
        response->clear_details();
        response->set_code(0);
        response->clear_milestone();

        if (request->has_user() && session_is_valid(request->user()))
        {
            if (request->subject_id() == 0)
            {
                response->set_details("invalid subject");
                response->set_code(1);
            }
            else if (request->roadmap_id() == 0)
            {
                response->set_details("invalid roadmap");
                response->set_code(2);
            }
            else
            {
                if (request->position() > 0)
                {
                    Milestone milestone = m_database->add_milestone(request->subject_id(), request->subject_level(), request->roadmap_id(), request->position());
                    response->set_allocated_milestone(std::make_unique<flashback::Milestone>(milestone).release());
                    response->set_success(true);
                }
                else
                {
                    Milestone milestone = m_database->add_milestone(request->subject_id(), request->subject_level(), request->roadmap_id());
                    response->set_allocated_milestone(std::make_unique<flashback::Milestone>(milestone).release());
                    response->set_success(true);
                }
            }
        }
    }
    catch (client_exception const& exp)
    {
        response->set_code(3);
        response->set_details(exp.what());
        response->set_success(false);
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
        response->set_code(3);
        response->set_details("internal error");
        response->set_success(false);
    }

    return grpc::Status::OK;
}

grpc::Status server::AddRequirement(grpc::ServerContext* context, AddRequirementRequest const* request, AddRequirementResponse* response)
{
    try
    {
        response->set_success(false);
        response->clear_details();
        response->set_code(0);

        if (request->has_user() && session_is_valid(request->user()))
        {
            m_database->add_requirement(request->roadmap().id(), request->milestone(), request->required_milestone());
            response->set_success(true);
        }
    }
    catch (client_exception const& exp)
    {
        response->set_code(3);
        response->set_details(exp.what());
        response->set_success(false);
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
        response->set_code(3);
        response->set_details("internal error");
        response->set_success(false);
    }

    return grpc::Status::OK;
}

grpc::Status server::GetRequirements(grpc::ServerContext* context, GetRequirementsRequest const* request, GetRequirementsResponse* response)
{
    try
    {
        response->set_success(false);
        response->clear_details();
        response->set_code(0);

        if (request->has_user() && session_is_valid(request->user()))
        {
            for (Milestone const& requirement: m_database->get_requirements(request->roadmap().id(), request->milestone().id(), request->milestone().level()))
            {
                Milestone* milestone = response->add_milestones();
                *milestone = requirement;
                response->set_success(true);
            }
        }
    }
    catch (client_exception const& exp)
    {
        response->set_code(3);
        response->set_details(exp.what());
        response->set_success(false);
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
        response->set_code(3);
        response->set_details("internal error");
        response->set_success(false);
    }
    return grpc::Status::OK;
}

grpc::Status server::CreateSubject(grpc::ServerContext* context, CreateSubjectRequest const* request, CreateSubjectResponse* response)
{
    try
    {
        if (request->has_user() && session_is_valid(request->user()))
        {
            m_database->create_subject(request->name());
            response->set_success(true);
            response->clear_details();
            response->set_code(0);
        }
    }
    catch (flashback::client_exception const& exp)
    {
        response->set_success(false);
        response->set_details(exp.what());
        response->set_code(1);
    }
    catch (pqxx::unique_violation const& exp)
    {
        response->set_success(false);
        response->set_details("duplicate request");
        response->set_code(2);
    }
    catch (std::exception const& exp)
    {
        response->set_success(false);
        response->set_details("internal error");
        response->set_code(3);
        std::cerr << std::format("Server: error while creating subject: {}", exp.what());
    }

    return grpc::Status::OK;
}

grpc::Status server::SearchSubjects(grpc::ServerContext* context, SearchSubjectsRequest const* request, SearchSubjectsResponse* response)
{
    try
    {
        if (request->has_user() && !request->token().empty() && session_is_valid(request->user()))
        {
            for (auto const& [position, subject]: m_database->search_subjects(request->token()))
            {
                MatchingSubject* matching_subject = response->add_subjects();
                matching_subject->set_position(position);
                matching_subject->set_allocated_subject(std::make_unique<Subject>(subject).release());
            }
        }
    }
    catch (client_exception const& exp)
    {
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: error while searching for subjects: {}", exp.what());
    }

    return grpc::Status::OK;
}

grpc::Status server::ReorderMilestone(grpc::ServerContext* context, ReorderMilestoneRequest const* request, ReorderMilestoneResponse* response)
{
    try
    {
        if (request->has_user() && session_is_valid(request->user()))
        {
            if (!request->has_roadmap() || request->roadmap().id() == 0)
            {
                response->set_success(false);
                response->set_details("invalid roadmap");
                response->set_code(4);
            }
            else if (request->current_position() == 0 || request->target_position() == 0)
            {
                response->set_success(false);
                response->set_details("invalid positions");
                response->set_code(5);
            }
            else if (request->current_position() == request->target_position())
            {
                response->set_success(false);
                response->set_details("same position is invalid");
                response->set_code(6);
            }
            else
            {
                m_database->reorder_milestone(request->roadmap().id(), request->current_position(), request->target_position());
                response->set_success(true);
                response->clear_details();
                response->set_code(0);
            }
        }
        else
        {
            response->set_success(false);
            response->set_details("invalid user");
            response->set_code(3);
        }
    }
    catch (flashback::client_exception const& exp)
    {
        response->set_success(false);
        response->set_details(exp.what());
        response->set_code(1);
    }
    catch (pqxx::unique_violation const& exp)
    {
        response->set_success(false);
        response->set_details("duplicate request");
        response->set_code(2);
    }
    catch (std::exception const& exp)
    {
        response->set_success(false);
        response->set_details("internal error");
        response->set_code(3);
        std::cerr << std::format("Server: error while creating subject: {}", exp.what());
    }

    return grpc::Status::OK;
}

grpc::Status server::RemoveMilestone(grpc::ServerContext* context, RemoveMilestoneRequest const* request, RemoveMilestoneResponse* response)
{
    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            response->set_success(false);
            response->set_details("invalid user");
            response->set_code(3);
        }
        else if (!request->has_roadmap() || request->roadmap().id() == 0)
        {
            response->set_success(false);
            response->set_details("invalid roadmap");
            response->set_code(4);
        }
        else if (!request->has_milestone() || request->milestone().id() == 0)
        {
            response->set_success(false);
            response->set_details("invalid milestone");
            response->set_code(5);
        }
        else
        {
            m_database->remove_milestone(request->roadmap().id(), request->milestone().id());
            response->set_success(true);
            response->clear_details();
            response->set_code(0);
        }
    }
    catch (client_exception const& exp)
    {
        response->set_success(false);
        response->set_details(exp.what());
        response->set_code(1);
    }
    catch (std::exception const& exp)
    {
        response->set_success(false);
        response->set_details("internal error");
        response->set_code(2);
    }

    return grpc::Status::OK;
}

grpc::Status server::ChangeMilestoneLevel(grpc::ServerContext* context, ChangeMilestoneLevelRequest const* request, ChangeMilestoneLevelResponse* response)
{
    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            response->set_success(false);
            response->set_details("invalid user");
            response->set_code(3);
        }
        else if (!request->has_roadmap() || request->roadmap().id() == 0)
        {
            response->set_success(false);
            response->set_details("invalid roadmap");
            response->set_code(4);
        }
        else if (!request->has_milestone() || request->milestone().id() == 0)
        {
            response->set_success(false);
            response->set_details("invalid milestone");
            response->set_code(5);
        }
        else
        {
            m_database->change_milestone_level(request->roadmap().id(), request->milestone().id(), request->milestone().level());
            response->set_success(true);
            response->clear_details();
            response->set_code(0);
        }
    }
    catch (client_exception const& exp)
    {
        response->set_success(false);
        response->set_details(exp.what());
        response->set_code(1);
    }
    catch (std::exception const& exp)
    {
        response->set_success(false);
        response->set_details("internal error");
        response->set_code(2);
    }

    return grpc::Status::OK;
}

grpc::Status server::RenameSubject(grpc::ServerContext* context, RenameSubjectRequest const* request, RenameSubjectResponse* response)
{
    try
    {
        if (request->has_user() && session_is_valid(request->user()))
        {
            if (request->id() == 0)
            {
                response->set_details("invalid subject id");
                response->set_code(1);
            }
            else if (request->name().empty())
            {
                response->set_details("invalid subject name");
                response->set_code(2);
            }
            else
            {
                m_database->rename_subject(request->id(), request->name());
                response->set_success(true);
                response->set_code(0);
            }
        }
        else
        {
            response->set_details("invalid user");
            response->set_code(3);
        }
    }
    catch (client_exception const& exp)
    {
        response->set_details(exp.what());
        response->set_code(4);
    }

    return grpc::Status::OK;
}

grpc::Status server::RemoveSubject(grpc::ServerContext* context, RemoveSubjectRequest const* request, RemoveSubjectResponse* response)
{
    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            response->set_success(false);
            response->set_details("invalid user");
            response->set_code(3);
        }
        else if (request->subject().id() == 0)
        {
            response->set_success(false);
            response->set_details("invalid subject");
            response->set_code(4);
        }
        else
        {
            m_database->remove_subject(request->subject().id());
            response->set_success(true);
            response->clear_details();
            response->set_code(0);
        }
    }
    catch (client_exception const& exp)
    {
        response->set_success(false);
        response->set_details(exp.what());
        response->set_code(1);
    }
    catch (std::exception const& exp)
    {
        response->set_success(false);
        response->set_details("internal error");
        response->set_code(2);
    }

    return grpc::Status::OK;
}

grpc::Status server::MergeSubjects(grpc::ServerContext* context, MergeSubjectsRequest const* request, MergeSubjectsResponse* response)
{
    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            response->set_success(false);
            response->set_details("invalid user");
            response->set_code(3);
        }
        else if (request->source_subject().id() == 0 || request->target_subject().id() == 0)
        {
            response->set_success(false);
            response->set_details("invalid subject");
            response->set_code(4);
        }
        else
        {
            m_database->merge_subjects(request->source_subject().id(), request->target_subject().id());
            response->set_success(true);
            response->clear_details();
            response->set_code(0);
        }
    }
    catch (client_exception const& exp)
    {
        response->set_success(false);
        response->set_details(exp.what());
        response->set_code(1);
    }
    catch (std::exception const& exp)
    {
        response->set_success(false);
        response->set_details("internal error");
        response->set_code(2);
    }

    return grpc::Status::OK;
}

grpc::Status server::GetResources(grpc::ServerContext* context, GetResourcesRequest const* request, GetResourcesResponse* response)
{
    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            response->set_success(false);
            response->set_details("invalid user");
            response->set_code(3);
        }
        else if (request->subject().id() == 0)
        {
            response->set_success(false);
            response->set_details("invalid subject");
            response->set_code(4);
        }
        else
        {
            for (Resource const& r: m_database->get_resources(request->user().id(), request->subject().id()))
            {
                Resource* resource = response->add_resources();
                *resource = r;
            }
            response->set_success(true);
            response->clear_details();
            response->set_code(0);
        }
    }
    catch (client_exception const& exp)
    {
        response->set_success(false);
        response->set_details(exp.what());
        response->set_code(1);
    }
    catch (std::exception const& exp)
    {
        response->set_success(false);
        response->set_details("internal error");
        response->set_code(2);
        std::cerr << std::format("Server: {}", exp.what());
    }

    return grpc::Status::OK;
}

grpc::Status server::CreateResource(grpc::ServerContext* context, CreateResourceRequest const* request, CreateResourceResponse* response)
{
    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            response->set_success(false);
            response->set_details("invalid user");
            response->set_code(3);
        }
        else if (request->resource().id() != 0)
        {
            response->set_success(false);
            response->set_details("resource already has an identifier");
            response->set_code(4);
        }
        else
        {
            auto resource{std::make_unique<Resource>(m_database->create_resource(request->resource()))};
            response->set_success(true);
            response->clear_details();
            response->set_code(0);
            response->set_allocated_resource(resource.release());
        }
    }
    catch (client_exception const& exp)
    {
        response->set_success(false);
        response->set_details(exp.what());
        response->set_code(1);
    }
    catch (std::exception const& exp)
    {
        response->set_success(false);
        response->set_details("internal error");
        response->set_code(2);
        std::cerr << std::format("Server: {}", exp.what());
    }

    return grpc::Status::OK;
}

grpc::Status server::AddResourceToSubject(grpc::ServerContext* context, AddResourceToSubjectRequest const* request, AddResourceToSubjectResponse* response)
{
    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            response->set_success(false);
            response->set_details("invalid user");
            response->set_code(3);
        }
        else if (request->resource().id() == 0)
        {
            response->set_success(false);
            response->set_details("invalid resource");
            response->set_code(4);
        }
        else if (request->subject().id() == 0)
        {
            response->set_success(false);
            response->set_details("invalid subject");
            response->set_code(4);
        }
        else
        {
            m_database->add_resource_to_subject(request->resource().id(), request->subject().id());
            response->set_success(true);
            response->clear_details();
            response->set_code(0);
        }
    }
    catch (client_exception const& exp)
    {
        response->set_success(false);
        response->set_details(exp.what());
        response->set_code(1);
    }
    catch (std::exception const& exp)
    {
        response->set_success(false);
        response->set_details("internal error");
        response->set_code(2);
        std::cerr << std::format("Server: {}", exp.what());
    }

    return grpc::Status::OK;
}

grpc::Status server::DropResourceFromSubject(grpc::ServerContext* context, DropResourceFromSubjectRequest const* request, DropResourceFromSubjectResponse* response)
{
    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            response->set_success(false);
            response->set_details("invalid user");
            response->set_code(3);
        }
        else if (request->resource().id() == 0)
        {
            response->set_success(false);
            response->set_details("invalid resource");
            response->set_code(4);
        }
        else if (request->subject().id() == 0)
        {
            response->set_success(false);
            response->set_details("invalid subject");
            response->set_code(4);
        }
        else
        {
            m_database->drop_resource_from_subject(request->resource().id(), request->subject().id());
            response->set_success(true);
            response->clear_details();
            response->set_code(0);
        }
    }
    catch (client_exception const& exp)
    {
        response->set_success(false);
        response->set_details(exp.what());
        response->set_code(1);
    }
    catch (std::exception const& exp)
    {
        response->set_success(false);
        response->set_details("internal error");
        response->set_code(2);
        std::cerr << std::format("Server: {}", exp.what());
    }

    return grpc::Status::OK;
}

grpc::Status server::SearchResources(grpc::ServerContext* context, SearchResourcesRequest const* request, SearchResourcesResponse* response)
{
    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            response->set_success(false);
            response->set_details("invalid user");
            response->set_code(3);
        }
        else if (request->search_token().empty())
        {
            response->set_success(false);
            response->set_details("invalid input");
            response->set_code(4);
        }
        else
        {
            for (auto const& [position, resource]: m_database->search_resources(request->search_token()))
            {
                ResourceSearchResult* result{response->add_results()};
                auto* allocated_resource{result->mutable_resource()};
                *allocated_resource = resource;
                result->set_position(position);
            }
            response->set_success(true);
            response->clear_details();
            response->set_code(0);
        }
    }
    catch (client_exception const& exp)
    {
        response->set_success(false);
        response->set_details(exp.what());
        response->set_code(1);
    }
    catch (std::exception const& exp)
    {
        response->set_success(false);
        response->set_details("internal error");
        response->set_code(2);
        std::cerr << std::format("Server: {}", exp.what());
    }

    return grpc::Status::OK;
}

grpc::Status server::MergeResources(grpc::ServerContext* context, MergeResourcesRequest const* request, MergeResourcesResponse* response)
{
    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            response->set_success(false);
            response->set_details("invalid user");
            response->set_code(3);
        }
        else if (request->source().id() == 0)
        {
            response->set_success(false);
            response->set_details("invalid source");
            response->set_code(4);
        }
        else if (request->target().id() == 0)
        {
            response->set_success(false);
            response->set_details("invalid target");
            response->set_code(5);
        }
        else
        {
            m_database->merge_resources(request->source().id(), request->target().id());
            response->set_success(true);
            response->clear_details();
            response->set_code(0);
        }
    }
    catch (client_exception const& exp)
    {
        response->set_success(false);
        response->set_details(exp.what());
        response->set_code(1);
    }
    catch (std::exception const& exp)
    {
        response->set_success(false);
        response->set_details("internal error");
        response->set_code(2);
        std::cerr << std::format("Server: {}", exp.what());
    }

    return grpc::Status::OK;
}

grpc::Status server::RemoveResource(grpc::ServerContext* context, RemoveResourceRequest const* request, RemoveResourceResponse* response)
{
    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            response->set_success(false);
            response->set_details("invalid user");
            response->set_code(3);
        }
        else if (request->resource().id() == 0)
        {
            response->set_success(false);
            response->set_details("invalid resource");
            response->set_code(4);
        }
        else
        {
            m_database->remove_resource(request->resource().id());
            response->set_success(true);
            response->clear_details();
            response->set_code(0);
        }
    }
    catch (client_exception const& exp)
    {
        response->set_success(false);
        response->set_details(exp.what());
        response->set_code(1);
    }
    catch (std::exception const& exp)
    {
        response->set_success(false);
        response->set_details("internal error");
        response->set_code(2);
        std::cerr << std::format("Server: {}", exp.what());
    }

    return grpc::Status::OK;
}

grpc::Status server::EditResource(grpc::ServerContext* context, EditResourceRequest const* request, EditResourceResponse* response)
{
    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            response->set_success(false);
            response->set_details("invalid user");
            response->set_code(3);
        }
        else if (request->resource().id() == 0)
        {
            response->set_success(false);
            response->set_details("invalid resource");
            response->set_code(4);
        }
        else
        {
            bool modified{};
            Resource resource{m_database->get_resource(request->resource().id())};

            if (resource.name() != request->resource().name())
            {
                m_database->rename_resource(request->resource().id(), request->resource().name());
                modified = true;
            }

            if (resource.link() != request->resource().link())
            {
                m_database->edit_resource_link(request->resource().id(), request->resource().link());
                modified = true;
            }

            if (resource.type() != request->resource().type())
            {
                m_database->change_resource_type(request->resource().type(), request->resource().type());
                modified = true;
            }

            if (resource.pattern() != request->resource().pattern())
            {
                m_database->change_section_pattern(request->resource().id(), request->resource().pattern());
                modified = true;
            }

            if (resource.production() != request->resource().production())
            {
                m_database->edit_resource_production(request->resource().id(), request->resource().production());
                modified = true;
            }

            if (resource.expiration() != request->resource().expiration())
            {
                m_database->edit_resource_expiration(request->resource().id(), request->resource().expiration());
                modified = true;
            }

            response->set_success(modified);

            if (modified)
            {
                response->clear_details();
                response->set_code(0);
            }
            else
            {
                response->set_details("no modification");
                response->set_code(5);
            }
        }
    }
    catch (client_exception const& exp)
    {
        response->set_success(false);
        response->set_details(exp.what());
        response->set_code(1);
    }
    catch (std::exception const& exp)
    {
        response->set_success(false);
        response->set_details("internal error");
        response->set_code(2);
        std::cerr << std::format("Server: {}", exp.what());
    }

    return grpc::Status::OK;
}

grpc::Status server::CreateNerve(grpc::ServerContext* context, CreateNerveRequest const* request, CreateNerveResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->resource().id() != 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid resource", "resource id must be zero before creation"};
        }
        else
        {
            auto nerve = response->mutable_resource();
            *nerve = m_database->create_nerve(request->user().id(), request->resource().name(), request->subject().id(), request->resource().expiration());

            if (nerve->id() == 0)
            {
                status = grpc::Status{grpc::StatusCode::NOT_FOUND, "nerve not found"};
            }
            else
            {
                status = grpc::Status{grpc::StatusCode::OK, {}};
            }
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::GetNerves(grpc::ServerContext* context, GetNervesRequest const* request, GetNervesResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else
        {
            for (auto const& nerve: m_database->get_nerves(request->user().id()))
            {
                *response->add_resources() = nerve;
            }
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::CreateProvider(grpc::ServerContext* context, CreateProviderRequest const* request, CreateProviderResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->provider().name().empty())
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid provider name"};
        }
        else if (request->provider().id() != 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid provider", "provider id must be zero before creation"};
        }
        else
        {
            Provider provider{m_database->create_provider(request->provider().name())};
            *response->mutable_provider() = provider;
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::AddProvider(grpc::ServerContext* context, AddProviderRequest const* request, AddProviderResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->resource().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid resource"};
        }
        else if (request->provider().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid provider"};
        }
        else
        {
            m_database->add_provider(request->resource().id(), request->provider().id());
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::DropProvider(grpc::ServerContext* context, DropProviderRequest const* request, DropProviderResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->resource().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid resource"};
        }
        else if (request->provider().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid provider"};
        }
        else
        {
            m_database->drop_provider(request->resource().id(), request->provider().id());
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::SearchProviders(grpc::ServerContext* context, SearchProvidersRequest const* request, SearchProvidersResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->search_token().empty())
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "empty search not possible"};
        }
        else
        {
            for (auto const& [position, provider]: m_database->search_providers(request->search_token()))
            {
                auto* result{response->mutable_result()};
                *result->mutable_provider() = provider;
                result->set_position(position);
            }
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::RenameProvider(grpc::ServerContext* context, RenameProviderRequest const* request, RenameProviderResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->provider().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid provider"};
        }
        else
        {
            m_database->rename_provider(request->provider().id(), request->provider().name());
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::RemoveProvider(grpc::ServerContext* context, RemoveProviderRequest const* request, RemoveProviderResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->provider().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid provider"};
        }
        else
        {
            m_database->remove_provider(request->provider().id());
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::MergeProviders(grpc::ServerContext* context, MergeProvidersRequest const* request, MergeProvidersResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->source().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid source provider"};
        }
        else if (request->target().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid target provider"};
        }
        else
        {
            m_database->merge_providers(request->source().id(), request->target().id());
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::CreatePresenter(grpc::ServerContext* context, CreatePresenterRequest const* request, CreatePresenterResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->presenter().name().empty())
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid presenter name"};
        }
        else if (request->presenter().id() != 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid presenter", "presenter id must be zero before creation"};
        }
        else
        {
            auto presenter{m_database->create_presenter(request->presenter().name())};
            *response->mutable_presenter() = presenter;
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::AddPresenter(grpc::ServerContext* context, AddPresenterRequest const* request, AddPresenterResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->resource().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid resource"};
        }
        else if (request->presenter().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid presenter"};
        }
        else
        {
            m_database->add_presenter(request->resource().id(), request->presenter().id());
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::DropPresenter(grpc::ServerContext* context, DropPresenterRequest const* request, DropPresenterResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->resource().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid resource"};
        }
        else if (request->presenter().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid presenter"};
        }
        else
        {
            m_database->drop_presenter(request->resource().id(), request->presenter().id());
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::SearchPresenters(grpc::ServerContext* context, SearchPresentersRequest const* request, SearchPresentersResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->search_token().empty())
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "empty search is not allowed"};
        }
        else
        {
            for (auto const& [position, presenter]: m_database->search_presenters(request->search_token()))
            {
                auto* result{response->mutable_result()};
                *result->mutable_presenter() = presenter;
                result->set_position(position);
            }
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::RenamePresenter(grpc::ServerContext* context, RenamePresenterRequest const* request, RenamePresenterResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->presenter().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid presenter"};
        }
        else
        {
            m_database->rename_presenter(request->resource().id(), request->presenter().name());
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::RemovePresenter(grpc::ServerContext* context, RemovePresenterRequest const* request, RemovePresenterResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->presenter().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid presenter"};
        }
        else
        {
            m_database->remove_presenter(request->resource().id());
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::MergePresenters(grpc::ServerContext* context, MergePresentersRequest const* request, MergePresentersResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->source().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid source presenter"};
        }
        else if (request->target().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid target presenter"};
        }
        else
        {
            m_database->merge_presenters(request->source().id(), request->target().id());
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::GetTopics(grpc::ServerContext* context, GetTopicsRequest const* request, GetTopicsResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->subject().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid subject"};
        }
        else
        {
            for (auto const& [position, topic]: m_database->get_topics(request->subject().id(), request->level()))
            {
                *response->add_topic() = topic;
            }
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::CreateTopic(grpc::ServerContext* context, CreateTopicRequest const* request, CreateTopicResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->subject().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid subject"};
        }
        else if (request->topic().name().empty())
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid topic name"};
        }
        else
        {
            Topic* topic{response->mutable_topic()};
            *topic = m_database->create_topic(request->subject().id(), request->topic().name(), request->topic().level(), request->topic().position());
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::ReorderTopic(grpc::ServerContext* context, ReorderTopicRequest const* request, ReorderTopicResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->subject().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid subject"};
        }
        else if (request->topic().position() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid source topic"};
        }
        else if (request->target().position() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid target topic"};
        }
        else
        {
            m_database->reorder_topic(request->subject().id(), request->topic().level(), request->topic().position(), request->target().position());
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::RemoveTopic(grpc::ServerContext* context, RemoveTopicRequest const* request, RemoveTopicResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->subject().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid subject"};
        }
        else if (request->topic().position() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid topic"};
        }
        else
        {
            m_database->remove_topic(request->subject().id(), request->topic().level(), request->topic().position());
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::MergeTopics(grpc::ServerContext* context, MergeTopicsRequest const* request, MergeTopicsResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->subject().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid subject"};
        }
        else if (request->source().position() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid target topic"};
        }
        else if (request->target().position() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid source topic"};
        }
        else
        {
            m_database->merge_topics(request->subject().id(), request->source().level(), request->source().position(), request->target().position());
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::EditTopic(grpc::ServerContext* context, EditTopicRequest const* request, EditTopicResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};
    bool modified{};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->subject().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid subject"};
        }
        else if (request->topic().position() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid source topic"};
        }
        else if (request->target().position() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid target topic"};
        }
        else if (request->target().name().empty())
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid target topic name"};
        }
        else
        {
            Topic topic{m_database->get_topic(request->subject().id(), request->topic().level(), request->topic().position())};

            if (request->target().name() != topic.name())
            {
                modified = true;
                m_database->rename_topic(request->subject().id(), request->topic().level(), request->topic().position(), request->topic().name());
            }

            if (request->target().name() != topic.name())
            {
                modified = true;
                m_database->change_topic_level(request->subject().id(), request->topic().position(), request->topic().level(), request->target().level());
            }

            if (modified)
            {
                status = grpc::Status{grpc::StatusCode::OK, {}};
            }
            else
            {
                status = grpc::Status{grpc::StatusCode::ALREADY_EXISTS, {}};
            }
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::MoveTopic(grpc::ServerContext* context, MoveTopicRequest const* request, MoveTopicResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->source_subject().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid source subject"};
        }
        else if (request->target_subject().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid target subject"};
        }
        else if (request->source_topic().position() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid source topic"};
        }
        else if (request->target_topic().position() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid target topic"};
        }
        else
        {
            m_database->move_topic(request->source_subject().id(), request->source_topic().level(), request->source_topic().position(), request->target_subject().id(),
                                   request->target_topic().position());
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::SearchTopics(grpc::ServerContext* context, SearchTopicsRequest const* request, SearchTopicsResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->subject().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid subject"};
        }
        else if (request->search_token().empty())
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "empty search not allowed"};
        }
        else
        {
            for (auto const& [position, topic]: m_database->search_topics(request->subject().id(), request->level(), request->search_token()))
            {
                TopicSearchResult result{};
                *result.mutable_topic() = topic;
                result.set_position(position);
            }
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::GetSections(grpc::ServerContext* context, GetSectionsRequest const* request, GetSectionsResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->resource().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid resource"};
        }
        else
        {
            for (auto const& [position, section]: m_database->get_sections(request->resource().id()))
            {
                *response->add_section() = section;
            }
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::CreateSection(grpc::ServerContext* context, CreateSectionRequest const* request, CreateSectionResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->resource().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid resource"};
        }
        else if (request->section().name().empty())
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid section name"};
        }
        else
        {
            Section* section{response->mutable_section()};
            *section = m_database->create_section(request->resource().id(), request->section().position(), request->section().name(), request->section().link());
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::ReorderSection(grpc::ServerContext* context, ReorderSectionRequest const* request, ReorderSectionResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->resource().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid resource"};
        }
        else if (request->source().position() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid source section"};
        }
        else if (request->target().position() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid target section"};
        }
        else
        {
            m_database->reorder_section(request->resource().id(), request->source().position(), request->target().position());
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::RemoveSection(grpc::ServerContext* context, RemoveSectionRequest const* request, RemoveSectionResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->resource().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid resource"};
        }
        else if (request->section().position() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid section"};
        }
        else
        {
            m_database->remove_section(request->resource().id(), request->section().position());
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::MergeSections(grpc::ServerContext* context, MergeSectionsRequest const* request, MergeSectionsResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->resource().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid resource"};
        }
        else if (request->source().position() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid source section"};
        }
        else if (request->target().position() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid target section"};
        }
        else
        {
            m_database->merge_sections(request->resource().id(), request->source().position(), request->target().position());
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::EditSection(grpc::ServerContext* context, EditSectionRequest const* request, EditSectionResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};
    bool modified{};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->resource().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid resource"};
        }
        else if (request->section().position() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid section"};
        }
        else
        {
            Section section{m_database->get_section(request->resource().id(), request->section().position())};

            if (request->section().name() != section.name())
            {
                modified = true;
                m_database->rename_section(request->resource().id(), request->section().position(), request->section().name());
            }

            if (request->section().link() != section.link())
            {
                modified = true;
                m_database->edit_section_link(request->resource().id(), request->section().position(), request->section().link());
            }

            if (modified)
            {
                status = grpc::Status{grpc::StatusCode::OK, {}};
            }
            else
            {
                status = grpc::Status{grpc::StatusCode::ALREADY_EXISTS, {}};
            }
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::MoveSection(grpc::ServerContext* context, MoveSectionRequest const* request, MoveSectionResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->source_resource().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid source resource"};
        }
        else if (request->target_resource().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid target resource"};
        }
        else if (request->source_section().position() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid source section"};
        }
        else if (request->target_section().position() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid target section"};
        }
        else
        {
            m_database->move_section(request->source_resource().id(), request->source_section().position(), request->target_resource().id(), request->target_section().position());
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::SearchSections(grpc::ServerContext* context, SearchSectionsRequest const* request, SearchSectionsResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->resource().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid resource"};
        }
        else if (request->search_token().empty())
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "empty search token not allowed"};
        }
        else
        {
            for (auto const& [position, section]: m_database->search_sections(request->resource().id(), request->search_token()))
            {
                SectionSearchResult* result{response->add_result()};
                *result->mutable_section() = section;
                result->set_position(position);
            }
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::CreateCard(grpc::ServerContext* context, CreateCardRequest const* request, CreateCardResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->card().id() != 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid card", "card id must be zero before creation"};
        }
        else if (request->card().headline().empty())
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "empty headline not allowed"};
        }
        else
        {
            Card* card{response->mutable_card()};
            *card = m_database->create_card(request->card());
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::AddCardToSection(grpc::ServerContext* context, AddCardToSectionRequest const* request, AddCardToSectionResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->card().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid card"};
        }
        else if (request->resource().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid resource"};
        }
        else if (request->section().position() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid section"};
        }
        else
        {
            m_database->add_card_to_section(request->card().id(), request->resource().id(), request->section().position());
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::AddCardToTopic(grpc::ServerContext* context, AddCardToTopicRequest const* request, AddCardToTopicResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->card().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid card"};
        }
        else if (request->subject().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid subject"};
        }
        else if (request->topic().position() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid topic"};
        }
        else
        {
            m_database->add_card_to_topic(request->card().id(), request->subject().id(), request->topic().position(), request->topic().level());
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::RemoveCard(grpc::ServerContext* context, RemoveCardRequest const* request, RemoveCardResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->card().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid card"};
        }
        else
        {
            m_database->remove_card(request->card().id());
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::MergeCards(grpc::ServerContext* context, MergeCardsRequest const* request, MergeCardsResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->source().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid source card"};
        }
        else if (request->target().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid target card"};
        }
        else if (request->target().headline().empty())
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "empty headline not allowed"};
        }
        else
        {
            m_database->merge_cards(request->source().id(), request->target().id(), request->target().headline());
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::SearchCards(grpc::ServerContext* context, SearchCardsRequest const* request, SearchCardsResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->subject().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid subject"};
        }
        else if (request->search_token().empty())
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "empty search token not allowed"};
        }
        else
        {
            for (auto const& [position, card]: m_database->search_cards(request->subject().id(), request->level(), request->search_token()))
            {
                CardSearchResult* result{response->add_result()};
                *result->mutable_card() = card;
                result->set_position(position);
            }
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::MoveCardToSection(grpc::ServerContext* context, MoveCardToSectionRequest const* request, MoveCardToSectionResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->card().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid card"};
        }
        else if (request->resource().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid resource"};
        }
        else if (request->source().position() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid source section"};
        }
        else if (request->target().position() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid target section"};
        }
        else
        {
            m_database->move_card_to_section(request->card().id(), request->resource().id(), request->source().position(), request->target().position());
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::MarkSectionAsReviewed(grpc::ServerContext* context, MarkSectionAsReviewedRequest const* request, MarkSectionAsReviewedResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->resource().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid resource"};
        }
        else if (request->section().position() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid section"};
        }
        else
        {
            m_database->mark_section_as_reviewed(request->resource().id(), request->section().position());
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::GetPracticeCards(grpc::ServerContext* context, GetPracticeCardsRequest const* request, GetPracticeCardsResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->roadmap().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid roadmap"};
        }
        else if (request->subject().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid subject"};
        }
        else if (request->topic().position() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid topic"};
        }
        else
        {
            for (Card const& card: m_database->get_practice_cards(request->user().id(), request->roadmap().id(), request->subject().id(), request->topic().level(),
                                                                  request->topic().position()))
            {
                *response->add_card() = card;
            }
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::MoveCardToTopic(grpc::ServerContext* context, MoveCardToTopicRequest const* request, MoveCardToTopicResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->card().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid card"};
        }
        else if (request->subject().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid subject"};
        }
        else if (request->topic().position() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid topic"};
        }
        else if (request->target_subject().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid target subject"};
        }
        else if (request->target_topic().position() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid target topic"};
        }
        else
        {
            m_database->move_card_to_topic(request->card().id(), request->subject().id(), request->topic().position(), request->topic().level(), request->target_subject().id(),
                                           request->target_topic().position(), request->target_topic().level());
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::CreateAssessment(grpc::ServerContext* context, CreateAssessmentRequest const* request, CreateAssessmentResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->card().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid card", "card must already be created as an assessment is only a reference to it"};
        }
        else if (request->subject().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid subject"};
        }
        else if (request->topic().position() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid topic"};
        }
        else
        {
            m_database->create_assessment(request->subject().id(), request->topic().level(), request->topic().position(), request->card().id());
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::GetAssessments(grpc::ServerContext* context, GetAssessmentsRequest const* request, GetAssessmentsResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->subject().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid subject"};
        }
        else if (request->topic().position() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid topic"};
        }
        else
        {
            for (Assessment assessment: m_database->get_assessments(request->user().id(), request->subject().id(), request->topic().level(), request->topic().position()))
            {
                *response->add_assessment() = assessment;
            }
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::ExpandAssessment(grpc::ServerContext* context, ExpandAssessmentRequest const* request, ExpandAssessmentResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->card().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid card"};
        }
        else if (request->subject().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid subject"};
        }
        else if (request->topic().position() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid topic"};
        }
        else
        {
            m_database->expand_assessment(request->card().id(), request->subject().id(), request->topic().level(), request->topic().position());
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::DiminishAssessment(grpc::ServerContext* context, DiminishAssessmentRequest const* request, DiminishAssessmentResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->card().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid card"};
        }
        else if (request->subject().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid subject"};
        }
        else if (request->topic().position() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid topic"};
        }
        else
        {
            m_database->diminish_assessment(request->card().id(), request->subject().id(), request->topic().level(), request->topic().position());
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::IsAssimilated(grpc::ServerContext* context, IsAssimilatedRequest const* request, IsAssimilatedResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->subject().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid subject"};
        }
        else if (request->topic().position() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid topic"};
        }
        else
        {
            response->set_is_assimilated(m_database->is_assimilated(request->user().id(), request->subject().id(), request->topic().level(), request->topic().position()));
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::EditCard(grpc::ServerContext* context, EditCardRequest const* request, EditCardResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->card().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid card"};
        }
        else
        {
            Card card{m_database->get_card(request->card().id())};

            if (request->card().headline() != card.headline())
            {
                m_database->edit_card_headline(request->card().id(), request->card().headline());
            }
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::CreateBlock(grpc::ServerContext* context, CreateBlockRequest const* request, CreateBlockResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->card().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid card"};
        }
        else if (request->block().content().empty())
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid content", "block content cannot be empty"};
        }
        else if (request->block().extension().empty())
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid extension", "block extension cannot be empty"};
        }
        else
        {
            Block* block{response->mutable_block()};
            *block = m_database->create_block(request->card().id(), request->block());
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::GetBlocks(grpc::ServerContext* context, GetBlocksRequest const* request, GetBlocksResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->card().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid card"};
        }
        else
        {
            for (auto const& [position, block]: m_database->get_blocks(request->card().id()))
            {
                *response->add_block() = block;
            }
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::RemoveBlock(grpc::ServerContext* context, RemoveBlockRequest const* request, RemoveBlockResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->card().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid card"};
        }
        else if (request->block().position() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid block"};
        }
        else
        {
            m_database->remove_block(request->card().id(), request->block().position());
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::EditBlock(grpc::ServerContext* context, EditBlockRequest const* request, EditBlockResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};
    bool modified{};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->card().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid card"};
        }
        else if (request->block().position() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid block"};
        }
        else if (request->block().content().empty())
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid content", "block content cannot be empty"};
        }
        else if (request->block().extension().empty())
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid extension", "block extension cannot be empty"};
        }
        else
        {
            Block block{m_database->get_block(request->card().id(), request->block().position())};

            if (request->block().type() != block.type())
            {
                modified = true;
                m_database->change_block_type(request->card().id(), request->block().position(), request->block().type());
            }

            if (request->block().extension() != block.extension())
            {
                modified = true;
                m_database->edit_block_extension(request->card().id(), request->block().position(), request->block().extension());
            }

            if (request->block().content() != block.content())
            {
                modified = true;
                m_database->edit_block_content(request->card().id(), request->block().position(), request->block().content());
            }

            if (request->block().metadata() != block.metadata())
            {
                modified = true;
                m_database->edit_block_metadata(request->card().id(), request->block().position(), request->block().metadata());
            }

            if (modified)
            {
                status = grpc::Status{grpc::StatusCode::OK, {}};
            }
            else
            {
                status = grpc::Status{grpc::StatusCode::ALREADY_EXISTS, {}};
            }
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::ReorderBlock(grpc::ServerContext* context, ReorderBlockRequest const* request, ReorderBlockResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->card().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid card"};
        }
        else if (request->block().position() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid source block"};
        }
        else if (request->target().position() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid target block"};
        }
        else
        {
            m_database->reorder_block(request->card().id(), request->block().position(), request->target().position());
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::MergeBlocks(grpc::ServerContext* context, MergeBlocksRequest const* request, MergeBlocksResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->card().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid card"};
        }
        else if (request->block().position() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid source block"};
        }
        else if (request->target().position() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid target block"};
        }
        else
        {
            m_database->merge_blocks(request->card().id(), request->block().position(), request->target().position());
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::SplitBlock(grpc::ServerContext* context, SplitBlockRequest const* request, SplitBlockResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->card().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid card"};
        }
        else if (request->block().position() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid block"};
        }
        else
        {
            for (auto const& [position, block]: m_database->split_block(request->card().id(), request->block().position()))
            {
                *response->add_block() = block;
            }
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::MarkCardAsReviewed(grpc::ServerContext* context, MarkCardAsReviewedRequest const* request, MarkCardAsReviewedResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->card().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid card"};
        }
        else
        {
            m_database->mark_card_as_reviewed(request->card().id());
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::Study(grpc::ServerContext* context, StudyRequest const* request, StudyResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->card().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid card"};
        }
        else if (request->duration() < 3)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid duration", "reading a card less than 3 seconds is not acceptable"};
        }
        else
        {
            m_database->study(request->user().id(), request->card().id(), std::chrono::seconds{request->duration()});
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::MakeProgress(grpc::ServerContext* context, MakeProgressRequest const* request, MakeProgressResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else if (request->card().id() == 0)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid card"};
        }
        else if (request->duration() < 3)
        {
            status = grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "invalid duration", "reading a card less than 3 seconds is not acceptable"};
        }
        else
        {
            m_database->make_progress(request->user().id(), request->card().id(), request->duration(), request->mode());
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

grpc::Status server::GetProgressWeight(grpc::ServerContext* context, GetProgressWeightRequest const* request, GetProgressWeightResponse* response)
{
    grpc::Status status{grpc::StatusCode::INTERNAL, {}};

    try
    {
        if (!request->has_user() || !session_is_valid(request->user()))
        {
            status = grpc::Status{grpc::StatusCode::UNAUTHENTICATED, "invalid user"};
        }
        else
        {
            for (Weight weight: m_database->get_progress_weight(request->user().id()))
            {
                *response->add_weight() = weight;
            }
            status = grpc::Status{grpc::StatusCode::OK, {}};
        }
    }
    catch (client_exception const& exp)
    {
        status = grpc::Status{grpc::StatusCode::UNAVAILABLE, exp.what()};
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return status;
}

std::string server::calculate_hash(std::string_view password)
{
    char buffer[crypto_pwhash_STRBYTES];

    if (crypto_pwhash_str(buffer, password.data(), password.size(), crypto_pwhash_OPSLIMIT_MODERATE, crypto_pwhash_MEMLIMIT_MODERATE) != 0)
    {
        throw std::runtime_error("Server: sodium cannot create hash");
    }

    return buffer;
}

bool server::password_is_valid(std::string_view hash, std::string_view password)
{
    return crypto_pwhash_str_verify(hash.data(), password.data(), password.size()) == 0;
}

std::string server::generate_token()
{
    unsigned char entropy[32];
    char token[64];

    randombytes_buf(entropy, sizeof(entropy));
    sodium_bin2base64(token, sizeof(token), entropy, sizeof(entropy), sodium_base64_VARIANT_ORIGINAL_NO_PADDING);

    return std::string{token};
}

bool server::session_is_valid(User const& user) const
{
    return nullptr != m_database->get_user(user.token(), user.device());
}
