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
