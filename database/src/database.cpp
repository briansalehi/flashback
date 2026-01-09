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

Roadmap database::create_roadmap(std::string name)
{
    if (name.empty())
    {
        throw client_exception("roadmap name is empty");
    }

    pqxx::result result{query("select create_roadmap($1)", name)};
    uint64_t id = result.at(0).at(0).as<uint64_t>();
    Roadmap roadmap{};
    roadmap.set_id(id);
    roadmap.set_name(std::move(name));

    return roadmap;
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

Subject database::create_subject(std::string name)
{
    if (name.empty())
    {
        throw client_exception("subject name cannot be empty");
    }

    auto const result{query("select create_subject($1) as id", name)};
    auto const id{result.at(0, 0).as<uint64_t>()};
    Subject subject{};
    subject.set_name(std::move(name));
    subject.set_id(id);
    return subject;
}

std::map<uint64_t, Subject> database::search_subjects(std::string name)
{
    std::map<uint64_t, Subject> subjects{};

    if (!name.empty())
    {
        if (pqxx::result const matches{query("select position, id, name from search_subjects($1)", std::move(name))}; !matches.empty())
        {
            for (pqxx::row const& row: matches)
            {
                uint64_t position{row.at("position").as<uint64_t>()};
                Subject subject{};
                subject.set_id(row.at("id").as<uint64_t>());
                subject.set_name(row.at("name").as<std::string>());
                subjects.insert({position, subject});
            }
        }
    }

    return subjects;
}

void database::rename_subject(uint64_t id, std::string name)
{
    if (name.empty())
    {
        throw client_exception("Subject name cannot be empty");
    }

    exec("call rename_subject($1, $2)", id, std::move(name));
}

Milestone database::add_milestone(uint64_t const subject_id, expertise_level const subject_level, uint64_t const roadmap_id) const
{
    std::string level;
    Milestone milestone;

    switch (subject_level)
    {
    case flashback::expertise_level::surface: level = "surface";
        break;
    case flashback::expertise_level::depth: level = "depth";
        break;
    case flashback::expertise_level::origin: level = "origin";
        break;
    default: throw client_exception{"invalid expertise level"};
    }

    pqxx::result const result = query("select add_milestone($1, $2, $3) as id", subject_id, level, roadmap_id);
    milestone.set_id(subject_id);
    milestone.set_position(result.at(0).at("id").as<uint64_t>());
    milestone.set_level(subject_level);

    return milestone;
}

void database::add_milestone(uint64_t const subject_id, expertise_level const subject_level, uint64_t const roadmap_id, uint64_t const position) const
{
    std::string level;

    switch (subject_level)
    {
    case flashback::expertise_level::surface: level = "surface";
        break;
    case flashback::expertise_level::depth: level = "depth";
        break;
    case flashback::expertise_level::origin: level = "origin";
        break;
    default: throw std::runtime_error{"invalid expertise level"};
    }

    exec("call add_milestone($1, $2, $3, $4)", subject_id, level, roadmap_id, position);
}

std::vector<Milestone> database::get_milestones(uint64_t roadmap_id) const
{
    std::vector<Milestone> milestones{};
    for (pqxx::row const& row: query("select level, position, id, name from get_milestones($1)", roadmap_id))
    {
        uint64_t const id = row.at("id").as<uint64_t>();
        uint64_t const position = row.at("position").as<uint64_t>();
        std::string const level = row.at("level").as<std::string>();
        std::string const name = row.at("name").as<std::string>();

        Milestone milestone{};
        milestone.set_id(id);
        milestone.set_name(name);
        milestone.set_position(position);

        if (level == "surface") milestone.set_level(expertise_level::surface);
        else if (level == "depth") milestone.set_level(expertise_level::depth);
        else if (level == "origin") milestone.set_level(expertise_level::origin);
        else throw std::runtime_error{"invalid expertise level"};

        milestones.push_back(milestone);
    }
    return milestones;
}

expertise_level database::get_user_cognitive_level(uint64_t user_id, uint64_t subject_id) const
{
    auto level{expertise_level::surface};

    for (pqxx::result const result{query("select get_user_cognitive_level($1, $2)", user_id, subject_id)}; pqxx::row row: result)
    {
        if (std::string const string_level{row.at(0).as<std::string>()}; string_level == "surface")
        {
            level = expertise_level::surface;
        }
        else if (string_level == "depth")
        {
            level = expertise_level::depth;
        }
        else if (string_level == "origin")
        {
            level = expertise_level::origin;
        }
        else
        {
            throw std::runtime_error{"invalid expertise level"};
        }
    }

    return level;
}
