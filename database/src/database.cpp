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
        m_connection = std::make_unique<pqxx::connection>(std::format("postgres://flashback@{}:{}/{}", address, port, name));
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

bool database::create_session(uint64_t user_id, std::string_view token, std::string_view device)
{
    bool result{};

    try
    {
        exec("call create_session($1, $2, $3)", user_id, token, device);
        result = true;
    }
    catch (std::exception const& exp)
    {
        std::cerr << exp.what() << std::endl;
    }

    return result;
}

uint64_t database::create_user(std::string_view name, std::string_view email, std::string_view hash)
{
    uint64_t user_id{};

    try
    {
        pqxx::result result{query("select * from create_user($1, $2, $3)", name, email, hash)};

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
    exec("call reset_password($1, $2)", user_id, hash);
}

bool database::user_exists(std::string_view email)
{
    bool exists{false};

    if (pqxx::result result{query("select * from user_exists($1)", email)}; !result.empty())
    {
        exists = result.at(0).at(0).as<bool>();
    }

    return exists;
}

std::unique_ptr<User> database::get_user(std::string_view email)
{
    std::unique_ptr<User> user{nullptr};

    if (pqxx::result result_set{query("select * from get_user($1)", email)}; !result_set.empty())
    {
        pqxx::row result{result_set.at(0)};
        user = std::make_unique<User>();

        user->set_id(result.at("id").as<uint64_t>());
        user->set_name(result.at("name").as<std::string>());
        user->set_email(result.at("email").as<std::string>());
        user->set_verified(result.at("verified").as<bool>());

        if (!result.at("hash").is_null()) user->set_hash(result.at("hash").as<std::string>());

        if (!result.at("token").is_null()) user->set_token(result.at("token").as<std::string>());

        if (!result.at("device").is_null()) user->set_device(result.at("device").as<std::string>());

        std::tm tm{};

        std::string date_str{result.at("joined").as<std::string>()};
        std::istringstream stream{date_str};

        stream >> std::get_time(&tm, "%Y-%m-%d");
        time_t epoch{std::mktime(&tm)};

        auto timestamp{std::make_unique<google::protobuf::Timestamp>(google::protobuf::util::TimeUtil::SecondsToTimestamp(epoch))};
        user->set_allocated_joined(timestamp.release());

        std::string state_str{result.at("state").as<std::string>()};
        if (state_str == "active") user->set_state(User::active);
        else if (state_str == "inactive") user->set_state(User::inactive);
        else if (state_str == "suspended") user->set_state(User::suspended);
        else if (state_str == "banned") user->set_state(User::banned);
        else throw std::runtime_error("unhandled user state");
    }

    return user;
}

std::unique_ptr<User> database::get_user(std::string_view token, std::string_view device)
{
    std::unique_ptr<User> user{nullptr};

    pqxx::result result_set{query("select * from get_user($1, $2)", token, device)};

    if (result_set.size() == 1)
    {
        pqxx::row result{result_set.at(0)};
        user = std::make_unique<User>();

        user->set_id(result.at("id").as<uint64_t>());
        user->set_name(result.at("name").as<std::string>());
        user->set_email(result.at("email").as<std::string>());
        user->set_verified(result.at("verified").as<bool>());

        if (!result.at("hash").is_null()) user->set_hash(result.at("hash").as<std::string>());

        if (!result.at("token").is_null()) user->set_token(result.at("token").as<std::string>());

        if (!result.at("device").is_null()) user->set_device(result.at("device").as<std::string>());

        std::tm tm{};

        std::string date_str{result.at("joined").as<std::string>()};
        std::istringstream stream{date_str};

        stream >> std::get_time(&tm, "%Y-%m-%d");
        time_t epoch{std::mktime(&tm)};

        auto timestamp{std::make_unique<google::protobuf::Timestamp>(google::protobuf::util::TimeUtil::SecondsToTimestamp(epoch))};
        user->set_allocated_joined(timestamp.release());

        std::string state_str{result.at("state").as<std::string>()};
        if (state_str == "active") user->set_state(User::active);
        else if (state_str == "inactive") user->set_state(User::inactive);
        else if (state_str == "suspended") user->set_state(User::suspended);
        else if (state_str == "banned") user->set_state(User::banned);
        else throw std::runtime_error("unhandled user state");
    }

    return user;
}

void database::revoke_session(uint64_t user_id, std::string_view token)
{
    exec("call revoke_session($1, $2)", user_id, token);
}

void database::revoke_sessions_except(uint64_t user_id, std::string_view token)
{
    exec("call revoke_sessions_except($1, $2)", user_id, token);
}

uint64_t database::create_roadmap(std::string_view name)
{
    if (name.empty())
    {
        throw client_exception("roadmap name is empty");
    }

    pqxx::result result{query("select create_roadmap($1)", name)};
    uint64_t roadmap_id = result.at(0).at(0).as<uint64_t>();

    return roadmap_id;
}

void database::assign_roadmap(uint64_t user_id, uint64_t roadmap_id)
{
    exec("call assign_roadmap($1, $2)", user_id, roadmap_id);
}

std::vector<Roadmap> database::get_roadmaps(uint64_t user_id)
{
    pqxx::result const result{query("select id, name from get_roadmaps($1) order by name", user_id)};
    std::vector<Roadmap> roadmaps{};

    for (pqxx::row row: result)
    {
        Roadmap roadmap{};
        roadmap.set_id(row.at("id").as<std::uint64_t>());
        roadmap.set_name(row.at("name").as<std::string>());
        roadmaps.push_back(std::move(roadmap));
    }

    return roadmaps;
}

void database::rename_roadmap(uint64_t roadmap_id, std::string_view modified_name)
{
    if (modified_name.empty())
    {
        throw client_exception("roadmap modified name is empty");
    }

    exec("call rename_roadmap($1, $2)", roadmap_id, modified_name);
}

void database::remove_roadmap(uint64_t roadmap_id)
{
    exec("call remove_roadmap($1)", roadmap_id);
}

std::vector<Roadmap> database::search_roadmaps(std::string_view token)
{
    std::vector<Roadmap> roadmaps;
    roadmaps.reserve(5);

    for (auto const result = query("select roadmap, name from search_roadmaps($1) order by similarity", token); pqxx::row row: result)
    {
        Roadmap roadmap{};
        roadmap.set_id(row.at("roadmap").as<uint64_t>());
        roadmap.set_name(row.at("name").as<std::string>());
        roadmaps.push_back(std::move(roadmap));
    }

    return roadmaps;
}
