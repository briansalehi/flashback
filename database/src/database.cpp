#include <format>
#include <iostream>
#include <chrono>
#include <flashback/database.hpp>
#include <flashback/exception.hpp>
#include <google/protobuf/util/time_util.h>

using namespace flashback;

database::database(std::string name, std::string address, std::string port)
    : m_connection{nullptr}
{
    try
    {
        m_connection = std::make_unique<pqxx::connection>(
            std::format("postgres://flashback@{}:{}/{}", address, port, name));
    }
    catch (pqxx::broken_connection const& exp)
    {
        std::cerr << exp.what() << std::endl;
        std::exit(1);
    }
    catch (pqxx::disk_full const& exp)
    {
        std::cerr << exp.what() << std::endl;
        std::exit(2);
    }
    catch (pqxx::argument_error const& exp)
    {
        std::cerr << exp.what() << std::endl;
        std::exit(3);
    }
    catch (pqxx::data_exception const& exp)
    {
        std::cerr << exp.what() << std::endl;
        std::exit(4);
    }
}

std::shared_ptr<Roadmaps> database::get_roadmaps(uint64_t user_id)
{
    pqxx::result const result{query(std::format("select id, name from get_roadmaps({})", user_id))};
    auto roadmaps{std::make_shared<Roadmaps>()};

    for (pqxx::row row : result)
    {
        Roadmap* r{roadmaps->add_roadmap()};
        r->set_id(row.at("id").as<std::uint64_t>());
        r->set_name(row.at("name").as<std::string>());
    }

    return roadmaps;
}

std::pair<bool, std::string> database::create_session(uint64_t user_id, std::string_view token, std::string_view device)
{
    std::pair<bool, std::string> result{false, {}};

    try
    {
        exec(std::format("call create_session({}, '{}', '{}')", user_id, token, device));
        result.first = true;
    }
    catch (std::exception const& exp)
    {
        result.second = "internal error";
        std::cerr << exp.what() << std::endl;
    }

    return result;
}

uint64_t database::create_user(std::string_view name, std::string_view email, std::string_view hash)
{
    uint64_t user_id{};

    try
    {
        pqxx::result result{query(std::format("select * from create_user('{}', '{}', '{}')", name, email, hash))};

        if (result.size() != 1)
        {
            throw std::runtime_error(std::format("Server: could not create user because no user id was returned for {}", email));
        }

        user_id = result.at(0).at(0).as<uint64_t>();
    }
    catch (std::exception const& exp)
    {
        std::cerr << exp.what() << std::endl;
    }

    return user_id;
}

void database::reset_password(uint64_t user_id, std::string_view hash)
{
    exec(std::format("call reset_password({}, '{}')", user_id, hash));
}

std::unique_ptr<User> database::get_user(uint64_t user_id, std::string_view device)
{
    std::unique_ptr<User> user{nullptr};

    pqxx::result result_set{query(std::format("select * from get_user({}, '{}'::character varying)", user_id, device))};

    if (result_set.size() == 1)
    {
        pqxx::row result{result_set.at(0)};
        user = std::make_unique<User>();

        user->set_id(result.at("id").as<uint64_t>());
        user->set_name(result.at("name").as<std::string>());
        user->set_email(result.at("email").as<std::string>());
        user->set_hash(result.at("hash").as<std::string>());
        user->set_token(result.at("token").as<std::string>());
        user->set_device(result.at("device").as<std::string>());
        user->set_verified(result.at("verified").as<bool>());

        std::tm tm{};

        std::string date_str{result.at("joined").as<std::string>()};
        std::istringstream stream{date_str};

        stream >> std::get_time(&tm, "%Y-%m-%d");
        time_t epoch{std::mktime(&tm)};

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
    }

    return user;
}

void database::revoke_sessions_except(uint64_t user_id, std::string_view token)
{
    exec(std::format("call revoke_sessions_except({}, '{}'::character varying)", user_id, token));
}

pqxx::result database::query(std::string_view statement)
{
    pqxx::work work{*m_connection};
    pqxx::result result{work.exec(statement)};
    work.commit();
    return result;
}

void database::exec(std::string_view statement)
{
    pqxx::work work{*m_connection};
    work.exec(statement);
    work.commit();
}
