#include <iostream>
#include <format>
#include <chrono>
#include <sstream>
#include <algorithm>
#include <flashback/server.hpp>
#include <sodium.h>
#include <google/protobuf/timestamp.pb.h>
#include <google/protobuf/util/time_util.h>

using namespace flashback;

server::server(std::shared_ptr<database> database)
    : m_database{database}
{
    if (sodium_init() < 0)
    {
        throw std::runtime_error("Server: sodium library cannot be initialized");
    }
}

grpc::Status server::GetRoadmaps(grpc::ServerContext* context, User const* request, Roadmaps* response)
{
    std::clog << std::format("Server: user {} requests for roadmaps\n", request->id());
    pqxx::result const result{m_database->query(std::format("select id, name from get_roadmaps({})", request->id()))};
    std::clog << std::format("Server: retrieving {} roadmaps for user {}\n", result.size(), request->id());

    for (pqxx::row row : result)
    {
        Roadmap* r{response->add_roadmaps()};
        r->set_id(row.at("id").as<std::uint64_t>());
        r->set_name(row.at("name").as<std::string>());
    }

    return grpc::Status::OK;
}

grpc::Status server::SignIn(grpc::ServerContext* context, const SignInRequest* request, SignInResponse* response)
{
    uint64_t const opslimit{crypto_pwhash_OPSLIMIT_MODERATE};
    size_t const memlimit{crypto_pwhash_MEMLIMIT_MODERATE};
    char buffer[crypto_pwhash_STRBYTES];

    try
    {
        if (crypto_pwhash_str(buffer, request->user().password().c_str(), request->user().password().size(), opslimit, memlimit) != 0)
        {
            throw std::runtime_error("Server: sodium cannot create hash");
        }

        std::string hash{buffer};

        m_database->exec(std::format("call create_session('{}', '{}', '{}', '{}')", request->user().email(), hash, "dummy-token", request->user().device()));

        pqxx::row result{m_database->query(std::format("select * from get_user('{}')", request->user().email())).one_row()};

        auto user{std::make_unique<User>()};
        user->set_id(result.at("id").as<uint64_t>());
        user->set_verified(result.at("verified").as<bool>());
        auto time_str{result.at("joined").as<std::string>()};
        std::replace(time_str.begin(), time_str.end(), ' ', 'T');
        std::chrono::sys_time<std::chrono::microseconds> ms{};
        std::istringstream stream{time_str};
        stream >> std::chrono::parse("%FT%T%Ez", ms);
        auto epoch{std::chrono::duration_cast<std::chrono::seconds>(ms.time_since_epoch()).count()};
        auto timestamp{std::make_unique<google::protobuf::Timestamp>(google::protobuf::util::TimeUtil::SecondsToTimestamp(epoch))};
        user->set_allocated_joined(timestamp.release());

        std::string state_str{result.at("state").as<std::string>()};
        if (state_str == "active")
            user->set_state(User::active);
        else if (state_str == "inactive")
            user->set_state(User::inactive);
        else if (state_str == "suspended")
            user->set_state(User::suspended);
        else if (state_str == "banned")
            user->set_state(User::banned);
        else
            throw std::runtime_error("unhandled user state");

        response->set_success(true);
        response->set_token(hash);
        response->set_details("signin successful");
        response->set_allocated_user(user.release());
    }
    catch (pqxx::data_exception const& exp)
    {
        response->set_success(false);
        response->set_details(exp.what());
    }
    catch (std::exception const& exp)
    {
        response->set_success(false);
        response->set_details("internal error");
        std::cerr << exp.what() << std::endl;
    }

    return grpc::Status::OK;
}

grpc::Status server::SignUp(grpc::ServerContext* context, const SignUpRequest* request, SignUpResponse* response)
{
    response->set_success(true);
    response->set_details("signup successful");

    return grpc::Status::OK;
}
