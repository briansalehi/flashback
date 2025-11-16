#include <format>
#include <iostream>
#include <cstdlib>
#include <flashback/database.hpp>
#include <google/protobuf/timestamp.pb.h>
#include <google/protobuf/util/time_util.h>

using namespace flashback;

database::database(std::string address, std::string port)
    : m_connection{nullptr}
{
    try
    {
        m_connection = std::make_unique<pqxx::connection>(
            std::format("postgres://flashback@{}:{}/flashback", address, port));
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

std::pair<bool, std::string> database::create_session(std::string_view email, std::string_view hash,
                                                      std::string_view token, std::string_view device)
{
    std::pair<bool, std::string> result;

    try
    {
        exec(std::format("call create_session('{}', '{}', '{}', '{}')", email, hash, token, device));
    }
    catch (pqxx::sql_error const& exp)
    {
        result.first = true;

        if (exp.sqlstate().starts_with("UD"))
        {
            result.second = exp.sqlstate();
        }
        else
        {
            result.second = "internal error";
            std::cerr << exp.sqlstate() << std::endl;
            std::cerr << exp.what() << std::endl;
        }
    }
    catch (std::exception const& exp)
    {
        result.first = false;
        result.second = "internal error";
        std::cerr << exp.what() << std::endl;
    }

    return result;
}

std::unique_ptr<User> database::get_user(std::string_view email)
{
    auto user{std::make_unique<User>()};

    pqxx::row result{query(std::format("select * from get_user('{}')", email)).one_row()};

    user->set_id(result.at("id").as<uint64_t>());
    user->set_verified(result.at("verified").as<bool>());

    auto time_str{result.at("joined").as<std::string>()};
    std::replace(time_str.begin(), time_str.end(), ' ', 'T');
    std::chrono::sys_time<std::chrono::microseconds> ms{};
    std::istringstream stream{time_str};
    stream >> std::chrono::parse("%FT%T%Ez", ms);
    auto epoch{std::chrono::duration_cast<std::chrono::seconds>(ms.time_since_epoch()).count()};
    auto timestamp{ std::make_unique<google::protobuf::Timestamp>(google::protobuf::util::TimeUtil::SecondsToTimestamp(epoch))};
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

    return user;
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
