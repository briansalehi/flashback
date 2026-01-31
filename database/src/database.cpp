#include <format>
#include <iostream>
#include <chrono>
#include <flashback/database.hpp>
#include <flashback/exception.hpp>
#include <google/protobuf/util/time_util.h>

using namespace flashback;

database::database(std::string client, std::string name, std::string address, std::string port): m_connection{nullptr}
{
    try
    {
        m_connection = std::make_unique<pqxx::connection>(std::format("postgres://{}@{}:{}/{}", client, address, port, name));
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

bool database::create_session(uint64_t const user_id, std::string_view token, std::string_view device) const
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
        result = false;
    }

    return result;
}

uint64_t database::create_user(std::string_view name, std::string_view email, std::string_view hash) const
{
    uint64_t user_id{};

    try
    {
        pqxx::result const result{query("select * from create_user($1, $2, $3)", name, email, hash)};

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

void database::reset_password(uint64_t const user_id, std::string_view hash) const
{
    exec("call reset_password($1, $2)", user_id, hash);
}

bool database::user_exists(std::string_view email) const
{
    bool exists{false};

    if (pqxx::result const result{query("select * from user_exists($1)", email)}; !result.empty())
    {
        exists = result.at(0).at(0).as<bool>();
    }

    return exists;
}

std::unique_ptr<User> database::get_user(std::string_view email) const
{
    std::unique_ptr<User> user{nullptr};

    if (pqxx::result const result_set{query("select * from get_user($1)", email)}; !result_set.empty())
    {
        pqxx::row result{result_set.at(0)};
        user = std::make_unique<User>();

        user->set_id(result.at("id").as<uint64_t>());
        user->set_name(result.at("name").as<std::string>());
        user->set_email(result.at("email").as<std::string>());
        user->set_verified(result.at("verified").as<bool>());

        if (!result.at("hash").is_null())
            user->set_hash(result.at("hash").as<std::string>());

        if (!result.at("token").is_null())
            user->set_token(result.at("token").as<std::string>());

        if (!result.at("device").is_null())
            user->set_device(result.at("device").as<std::string>());

        std::tm tm{};
        std::istringstream stream{result.at("joined").as<std::string>()};

        stream >> std::get_time(&tm, "%Y-%m-%d");
        time_t epoch{std::mktime(&tm)};

        auto timestamp{std::make_unique<google::protobuf::Timestamp>(google::protobuf::util::TimeUtil::SecondsToTimestamp(epoch))};
        user->set_allocated_joined(timestamp.release());

        if (std::string const state_str{result.at("state").as<std::string>()}; state_str == "active")
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

std::unique_ptr<User> database::get_user(std::string_view token, std::string_view device) const
{
    std::unique_ptr<User> user{nullptr};

    if (pqxx::result const result_set{query("select * from get_user($1, $2)", token, device)}; result_set.size() == 1)
    {
        pqxx::row result{result_set.at(0)};
        user = std::make_unique<User>();

        user->set_id(result.at("id").as<uint64_t>());
        user->set_name(result.at("name").as<std::string>());
        user->set_email(result.at("email").as<std::string>());
        user->set_verified(result.at("verified").as<bool>());

        if (!result.at("hash").is_null())
            user->set_hash(result.at("hash").as<std::string>());

        if (!result.at("token").is_null())
            user->set_token(result.at("token").as<std::string>());

        if (!result.at("device").is_null())
            user->set_device(result.at("device").as<std::string>());

        std::tm tm{};
        std::istringstream stream{result.at("joined").as<std::string>()};

        stream >> std::get_time(&tm, "%Y-%m-%d");
        time_t epoch{std::mktime(&tm)};

        auto timestamp{std::make_unique<google::protobuf::Timestamp>(google::protobuf::util::TimeUtil::SecondsToTimestamp(epoch))};
        user->set_allocated_joined(timestamp.release());

        if (std::string const state_str{result.at("state").as<std::string>()}; state_str == "active")
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

void database::revoke_session(uint64_t const user_id, std::string_view token) const
{
    exec("call revoke_session($1, $2)", user_id, token);
}

void database::revoke_sessions_except(uint64_t const user_id, std::string_view token) const
{
    exec("call revoke_sessions_except($1, $2)", user_id, token);
}

Roadmap database::create_roadmap(uint64_t const user_id, std::string name) const
{
    if (name.empty())
    {
        throw client_exception("roadmap name is empty");
    }

    pqxx::result const result{query("select create_roadmap($1, $2)", user_id, name)};
    uint64_t const roadmap_id{result.at(0).at(0).as<uint64_t>()};
    Roadmap roadmap{};
    roadmap.set_id(roadmap_id);
    roadmap.set_name(std::move(name));

    return roadmap;
}

std::vector<Roadmap> database::get_roadmaps(uint64_t const user_id) const
{
    std::vector<Roadmap> roadmaps{};

    for (pqxx::result const result{query("select id, name from get_roadmaps($1) order by name", user_id)}; pqxx::row row: result)
    {
        Roadmap roadmap{};
        roadmap.set_id(row.at("id").as<std::uint64_t>());
        roadmap.set_name(row.at("name").as<std::string>());
        roadmaps.push_back(std::move(roadmap));
    }

    return roadmaps;
}

void database::rename_roadmap(uint64_t const roadmap_id, std::string_view modified_name) const
{
    if (modified_name.empty())
    {
        throw client_exception("roadmap modified name is empty");
    }

    exec("call rename_roadmap($1, $2)", roadmap_id, modified_name);
}

void database::remove_roadmap(uint64_t const roadmap_id) const
{
    exec("call remove_roadmap($1)", roadmap_id);
}

std::map<uint64_t, Roadmap> database::search_roadmaps(std::string_view search_pattern) const
{
    std::map<uint64_t, Roadmap> roadmaps;

    if (!search_pattern.empty())
    {
        for (auto const result = query("select similarity, roadmap, name from search_roadmaps($1) order by similarity", search_pattern); pqxx::row const& row: result)
        {
            uint64_t const similarity{row.at("similarity").as<uint64_t>()};
            Roadmap roadmap{};
            roadmap.set_id(row.at("roadmap").as<uint64_t>());
            roadmap.set_name(row.at("name").as<std::string>());
            roadmaps.insert({similarity, std::move(roadmap)});
        }
    }

    return roadmaps;
}

Subject database::create_subject(std::string name) const
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

std::map<uint64_t, Subject> database::search_subjects(std::string_view search_pattern) const
{
    std::map<uint64_t, Subject> subjects{};

    if (!search_pattern.empty())
    {
        for (pqxx::row const& row: query("select similarity, id, name from search_subjects($1) order by similarity", search_pattern))
        {
            uint64_t const similarity{row.at("similarity").as<uint64_t>()};
            Subject subject{};
            subject.set_id(row.at("id").as<uint64_t>());
            subject.set_name(row.at("name").as<std::string>());
            subjects.insert({similarity, subject});
        }
    }

    return subjects;
}

void database::rename_subject(uint64_t const subject_id, std::string name) const
{
    if (name.empty())
    {
        throw client_exception("Subject name cannot be empty");
    }

    exec("call rename_subject($1, $2)", subject_id, std::move(name));
}

void database::remove_subject(uint64_t const subject_id) const
{
    exec("call remove_subject($1)", subject_id);
}

void database::merge_subjects(uint64_t source, uint64_t target) const
{
}

Milestone database::add_milestone(uint64_t const subject_id, expertise_level const subject_level, uint64_t const roadmap_id) const
{
    Milestone milestone;
    pqxx::result const result = query("select add_milestone($1, $2, $3) as id", subject_id, level_to_string(subject_level), roadmap_id);
    milestone.set_id(subject_id);
    milestone.set_position(result.at(0).at("id").as<uint64_t>());
    milestone.set_level(subject_level);

    return milestone;
}

Milestone database::add_milestone(uint64_t const subject_id, expertise_level const subject_level, uint64_t const roadmap_id, uint64_t const position) const
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
    default: throw std::runtime_error{"invalid expertise level"};
    }

    exec("call add_milestone($1, $2, $3, $4)", subject_id, level, roadmap_id, position);
    milestone.set_id(subject_id);
    milestone.set_position(position);
    milestone.set_level(subject_level);

    return milestone;
}

std::vector<Milestone> database::get_milestones(uint64_t const roadmap_id) const
{
    std::vector<Milestone> milestones{};

    for (pqxx::row const& row: query("select level, position, id, name from get_milestones($1) order by position", roadmap_id))
    {
        uint64_t const id = row.at("id").as<uint64_t>();
        uint64_t const position = row.at("position").as<uint64_t>();
        std::string const level = row.at("level").as<std::string>();
        std::string const name = row.at("name").as<std::string>();

        Milestone milestone{};
        milestone.set_id(id);
        milestone.set_name(name);
        milestone.set_position(position);
        milestone.set_level(to_level(level));
        milestones.push_back(milestone);
    }

    return milestones;
}

void database::add_requirement(uint64_t const roadmap_id, Milestone const milestone, Milestone const required_milestone) const
{
    exec("call add_requirement($1, $2, $3, $4, $5)", roadmap_id, milestone.id(), level_to_string(milestone.level()), required_milestone.id(),
         level_to_string(required_milestone.level()));
}

std::vector<Milestone> database::get_requirements(uint64_t const roadmap_id, uint64_t const subject_id, expertise_level const subject_level) const
{
    std::vector<Milestone> requirements{};

    for (auto const& row: query("select subject, position, name, required_level from get_requirements($1, $2, $3)", roadmap_id, subject_id, level_to_string(subject_level)))
    {
        Milestone milestone;
        milestone.set_id(row.at("subject").as<uint64_t>());
        milestone.set_position(row.at("position").as<uint64_t>());
        milestone.set_name(row.at("name").as<std::string>());
        milestone.set_level(to_level(row.at("required_level").as<std::string>()));
        requirements.push_back(milestone);
    }

    return requirements;
}

Roadmap database::clone_roadmap(uint64_t const user_id, uint64_t const roadmap_id) const
{
    Roadmap roadmap{};
    roadmap.clear_id();
    roadmap.clear_name();

    for (pqxx::row const& row: query("select id, name from clone_roadmap($1, $2)", user_id, roadmap_id))
    {
        roadmap.set_id(row.at("id").as<uint64_t>());
        roadmap.set_name(row.at("name").as<std::string>());
    }

    return roadmap;
}

void database::reorder_milestone(uint64_t const roadmap_id, uint64_t const current_position, uint64_t const target_position) const
{
    exec("call reorder_milestone($1, $2, $3)", roadmap_id, current_position, target_position);
}

void database::remove_milestone(uint64_t roadmap_id, uint64_t subject_id) const
{
    exec("call remove_milestone($1, $2)", roadmap_id, subject_id);
}

void database::change_milestone_level(uint64_t const roadmap_id, uint64_t const subject_id, expertise_level const level) const
{
    exec("call change_milestone_level($1, $2, $3)", roadmap_id, subject_id, level_to_string(level));
}

Resource database::create_resource(Resource const& resource) const
{
    Resource generated_resource{resource};

    if (!resource.name().empty())
    {
        pqxx::row const row{
            query("select create_resource($1, $2, $3, $4, $5, $6) as id", resource.name(), resource_type_to_string(resource.type()), section_pattern_to_string(resource.pattern()),
                  resource.link(), resource.production(), resource.expiration()).at(0)
        };
        generated_resource.set_id(row.at("id").as<uint64_t>());
    }

    return generated_resource;
}

void database::add_resource_to_subject(uint64_t const resource_id, uint64_t const subject_id) const
{
    exec("call add_resource_to_subject($1, $2)", resource_id, subject_id);
}

std::vector<Resource> database::get_resources(uint64_t const subject_id) const
{
    std::vector<Resource> resources{};

    for (pqxx::row const& row: query("select id, name, type, pattern, link, production, expiration from get_resources($1)", subject_id))
    {
        Resource resource{};
        resource.set_id(row.at("id").as<uint64_t>());
        resource.set_name(row.at("name").as<std::string>());
        resource.set_type(to_resource_type(row.at("type").as<std::string>()));
        resource.set_pattern(to_section_pattern(row.at("pattern").as<std::string>()));
        resource.set_link(row.at("link").is_null() ? "" : row.at("link").as<std::string>());
        resource.set_production(row.at("production").as<uint64_t>());
        resource.set_expiration(row.at("expiration").as<uint64_t>());
        resources.push_back(resource);
    }

    return resources;
}

void database::drop_resource_from_subject(uint64_t const resource_id, uint64_t const subject_id) const
{
    exec("call drop_resource_from_subject($1, $2)", resource_id, subject_id);
}

std::map<uint64_t, Resource> database::search_resources(std::string_view search_pattern) const
{
    std::map<uint64_t, Resource> matched{};

    if (!search_pattern.empty())
    {
        for (pqxx::row const& row: query("select similarity, id, name, type, pattern, link, production, expiration from search_resources($1) order by similarity", search_pattern))
        {
            uint64_t const similarity{row.at("similarity").as<uint64_t>()};
            Resource resource{};
            resource.set_id(row.at("id").as<uint64_t>());
            resource.set_name(row.at("name").as<std::string>());
            resource.set_type(to_resource_type(row.at("type").as<std::string>()));
            resource.set_pattern(to_section_pattern(row.at("pattern").as<std::string>()));
            resource.set_link(row.at("link").is_null() ? "" : row.at("link").as<std::string>());
            resource.set_production(row.at("production").as<uint64_t>());
            resource.set_expiration(row.at("expiration").as<uint64_t>());
            matched.insert({similarity, resource});
        }
    }

    return matched;
}

void database::edit_resource_link(uint64_t const resource_id, std::string const link) const
{
    exec("call edit_resource_link($1, $2)", resource_id, link);
}

void database::change_resource_type(uint64_t const resource_id, Resource::resource_type const type) const
{
    exec("call change_resource_type($1, $2)", resource_id, resource_type_to_string(type));
}

void database::change_section_pattern(uint64_t const resource_id, Resource::section_pattern const pattern) const
{
    exec("call change_section_pattern($1, $2)", resource_id, section_pattern_to_string(pattern));
}

void database::edit_resource_production(uint64_t const resource_id, uint64_t const production) const
{
    exec("call edit_resource_production($1, $2)", resource_id, production);
}

void database::edit_resource_expiration(uint64_t const resource_id, uint64_t const expiration) const
{
    exec("call edit_resource_expiration($1, $2)", resource_id, expiration);
}

void database::rename_resource(uint64_t const resource_id, std::string name) const
{
    exec("call rename_resource($1, $2)", resource_id, name);
}

void database::remove_resource(uint64_t const resource_id) const
{
    exec("call remove_resource($1)", resource_id);
}

void database::merge_resources(uint64_t const source_id, uint64_t const target_id) const
{
    exec("call merge_resources($1, $2)", source_id, target_id);
}

Section database::create_section(uint64_t const resource_id, uint64_t const position, std::string name, std::string link) const
{
    Section section{};
    section.set_name(name);
    section.set_position(position);
    section.set_link(link);
    pqxx::result result{};

    if (position > 0)
    {
        exec("call create_section($1, $2, $3, $4)", resource_id, name, link, position);
    }
    else
    {
        if (result = query("select create_section($1, $2, $3) as id", resource_id, name, link); result.size() == 1)
        {
            section.set_position(result.at(0).at("id").as<uint64_t>());
        }
    }

    return section;
}

std::map<uint64_t, Section> database::get_sections(uint64_t const resource_id) const
{
    std::map<uint64_t, Section> sections{};

    for (pqxx::row const& row: query("select position, name, link from get_sections($1) order by position", resource_id))
    {
        Section section{};
        section.set_position(row.at("position").as<uint64_t>());
        section.set_name(row.at("name").as<std::string>());
        section.set_link(row.at("link").is_null() ? "" : row.at("link").as<std::string>());
        sections.insert({section.position(), section});
    }

    return sections;
}

void database::remove_section(uint64_t const resource_id, uint64_t const position) const
{
    exec("call remove_section($1, $2)", resource_id, position);
}

void database::reorder_section(uint64_t const resource_id, uint64_t const current_position, uint64_t const target_position) const
{
    exec("call reorder_section($1, $2, $3)", resource_id, current_position, target_position);
}

void database::merge_sections(uint64_t const resource_id, uint64_t const source_position, uint64_t const target_position) const
{
    exec("call merge_sections($1, $2, $3)", resource_id, source_position, target_position);
}

void database::rename_section(uint64_t const resource_id, uint64_t const position, std::string name) const
{
    exec("call rename_section($1, $2, $3)", resource_id, position, name);
}

void database::move_section(uint64_t const resource_id, uint64_t const position, uint64_t const target_resource_id, uint64_t const target_position) const
{
    exec("call move_section($1, $2, $3, $4)", resource_id, position, target_resource_id, target_position);
}

std::map<uint64_t, Section> database::search_sections(uint64_t const resource_id, std::string_view search_pattern) const
{
    std::map<uint64_t, Section> sections{};

    if (!search_pattern.empty())
    {
        for (pqxx::row const& row: query("select similarity, position, name, link from search_sections($1, $2) order by similarity", resource_id, search_pattern))
        {
            uint64_t const similarity{row.at("similarity").as<uint64_t>()};
            Section section{};
            section.set_position(row.at("position").as<uint64_t>());
            section.set_name(row.at("name").as<std::string>());
            section.set_link(row.at("link").is_null() ? "" : row.at("link").as<std::string>());
            sections.insert({similarity, section});
        }
    }

    return sections;
}

void database::edit_section_link(uint64_t resource_id, uint64_t position, std::string link) const
{
    exec("call edit_section_link($1, $2, $3)", resource_id, position, link);
}

Topic database::create_topic(uint64_t const subject_id, std::string name, expertise_level const level, uint64_t const position) const
{
    Topic topic{};
    topic.set_name(name);
    topic.set_level(level);
    topic.set_position(position);
    pqxx::result result{};

    if (position > 0)
    {
        exec("call create_topic($1, $2, $3, $4)", subject_id, level_to_string(level), std::move(name), position);
    }
    else
    {
        if (result = query("select create_topic($1, $2, $3) as id", subject_id, level_to_string(level), std::move(name)); result.size() == 1)
        {
            topic.set_position(result.at(0).at("id").as<uint64_t>());
        }
    }

    return topic;
}

std::map<uint64_t, Topic> database::get_topics(uint64_t const subject_id, expertise_level const level) const
{
    std::map<uint64_t, Topic> topics{};

    for (pqxx::row const& row: query("select position, name, level from get_topics($1, $2) order by position", subject_id, level_to_string(level)))
    {
        Topic topic{};
        topic.set_position(row.at("position").as<uint64_t>());
        topic.set_name(row.at("name").as<std::string>());
        topic.set_level(to_level(row.at("level").as<std::string>()));
        topics.insert({topic.position(), topic});
    }

    return topics;
}

void database::reorder_topic(uint64_t const subject_id, expertise_level const level, uint64_t const source_position, uint64_t const target_position) const
{
    exec("call reorder_topic($1, $2, $3, $4)", subject_id, level_to_string(level), source_position, target_position);
}

void database::remove_topic(uint64_t const subject_id, expertise_level const level, uint64_t const position) const
{
    exec("call remove_topic($1, $2, $3)", subject_id, level_to_string(level), position);
}

void database::merge_topics(uint64_t const subject_id, expertise_level const level, uint64_t const source_position, uint64_t const target_position) const
{
    exec("call merge_topics($1, $2, $3, $4)", subject_id, level_to_string(level), source_position, target_position);
}

void database::rename_topic(uint64_t const subject_id, expertise_level const level, uint64_t const position, std::string name) const
{
    exec("call rename_topic($1, $2, $3, $4)", subject_id, level_to_string(level), position, name);
}

void database::move_topic(uint64_t const subject_id, expertise_level const level, uint64_t const position, uint64_t const target_subject_id, uint64_t const target_position) const
{
    exec("call move_topic($1, $2, $3, $4, $5)", subject_id, level_to_string(level), position, target_subject_id, target_position);
}

std::map<uint64_t, Topic> database::search_topics(uint64_t const subject_id, expertise_level const level, std::string_view search_pattern) const
{
    std::map<uint64_t, Topic> topics{};

    if (!search_pattern.empty())
    {
        for (pqxx::row const& row: query("select similarity, position, name, level from search_topics($1, $2, $3) order by similarity", subject_id, level_to_string(level),
                                         search_pattern))
        {
            uint64_t const similarity{row.at("similarity").as<uint64_t>()};
            Topic topic{};
            topic.set_position(row.at("position").as<uint64_t>());
            topic.set_name(row.at("name").as<std::string>());
            topic.set_level(to_level(row.at("level").as<std::string>()));
            topics.insert({similarity, topic});
        }
    }

    return topics;
}

void database::change_topic_level(uint64_t const subject_id, uint64_t const position, flashback::expertise_level const level, flashback::expertise_level const target) const
{
    exec("call change_topic_level($1, $2, $3, $4)", subject_id, position, level_to_string(level), level_to_string(target));
}

Provider database::create_provider(std::string name) const
{
    Provider provider{};
    provider.clear_name();
    provider.clear_id();

    if (!name.empty())
    {
        if (pqxx::result const result{query("select create_provider($1) as id", name)}; result.size() == 1)
        {
            provider.set_id(result.at(0).at("id").as<uint64_t>());
            provider.set_name(std::move(name));
        }
    }

    return provider;
}

void database::add_provider(uint64_t const resource_id, uint64_t const provider_id) const
{
    exec("call add_provider($1, $2)", resource_id, provider_id);
}

void database::drop_provider(uint64_t const resource_id, uint64_t const provider_id) const
{
    exec("call drop_provider($1, $2)", resource_id, provider_id);
}

std::map<uint64_t, Provider> database::search_providers(std::string_view search_pattern) const
{
    std::map<uint64_t, Provider> matched{};

    if (!search_pattern.empty())
    {
        for (pqxx::row const& row: query("select similarity, provider, name from search_providers($1) order by similarity", search_pattern))
        {
            uint64_t const similarity{row.at("similarity").as<uint64_t>()};
            Provider provider{};
            provider.set_id(row.at("provider").as<uint64_t>());
            provider.set_name(row.at("name").as<std::string>());
            matched.insert({similarity, provider});
        }
    }

    return matched;
}

void database::rename_provider(uint64_t const provider_id, std::string name) const
{
    exec("call rename_provider($1, $2)", provider_id, std::move(name));
}

void database::remove_provider(uint64_t const provider_id) const
{
    exec("call remove_provider($1)", provider_id);
}

void database::merge_providers(uint64_t const source_id, uint64_t const target_id) const
{
    exec("call merge_providers($1, $2)", source_id, target_id);
}

Presenter database::create_presenter(std::string name) const
{
    Presenter presenter{};
    presenter.clear_name();
    presenter.clear_id();

    if (!name.empty())
    {
        if (pqxx::result const result{query("select create_presenter($1) as id", name)}; result.size() == 1)
        {
            presenter.set_id(result.at(0).at("id").as<uint64_t>());
            presenter.set_name(std::move(name));
        }
    }

    return presenter;
}

void database::add_presenter(uint64_t const resource_id, uint64_t const presenter_id) const
{
    exec("call add_presenter($1, $2)", resource_id, presenter_id);
}

void database::drop_presenter(uint64_t const resource_id, uint64_t const presenter_id) const
{
    exec("call drop_presenter($1, $2)", resource_id, presenter_id);
}

std::map<uint64_t, Presenter> database::search_presenters(std::string_view search_pattern) const
{
    std::map<uint64_t, Presenter> matched{};

    if (!search_pattern.empty())
    {
        for (pqxx::row const& row: query("select similarity, presenter, name from search_presenters($1) order by similarity", search_pattern))
        {
            uint64_t const similarity{row.at("similarity").as<uint64_t>()};
            Presenter presenter{};
            presenter.set_id(row.at("presenter").as<uint64_t>());
            presenter.set_name(row.at("name").as<std::string>());
            matched.insert({similarity, presenter});
        }
    }

    return matched;
}

void database::rename_presenter(uint64_t const presenter_id, std::string name) const
{
    exec("call rename_presenter($1, $2)", presenter_id, std::move(name));
}

void database::remove_presenter(uint64_t const presenter_id) const
{
    exec("call remove_presenter($1)", presenter_id);
}

void database::merge_presenters(uint64_t const source_id, uint64_t const target_id) const
{
    exec("call merge_presenters($1, $2)", source_id, target_id);
}

Card database::create_card(Card card) const
{
    if (!card.headline().empty())
    {
        if (pqxx::result const result{query("select create_card($1) as id", card.headline())}; result.size() == 1)
        {
            card.set_id(result.at(0).at("id").as<uint64_t>());
        }
    }
    return card;
}

void database::add_card_to_section(uint64_t const card_id, uint64_t const resource_id, uint64_t const section_position) const
{
    exec("select add_card_to_section($1, $2, $3) as position", card_id, resource_id, section_position);
}

void database::add_card_to_topic(uint64_t const card_id, uint64_t const subject_id, uint64_t const topic_position, expertise_level const topic_level) const
{
    exec("select add_card_to_topic($1, $2, $3, $4) as position", card_id, subject_id, topic_position, level_to_string(topic_level));
}

void database::edit_card_headline(uint64_t const card_id, std::string headline) const
{
    exec("call edit_card_headline($1, $2)", card_id, headline);
}

void database::remove_card(uint64_t const card_id) const
{
    exec("call remove_card($1)", card_id);
}

void database::merge_cards(uint64_t const source_id, uint64_t const target_id, std::string headline) const
{
    exec("call merge_cards($1, $2, $3)", source_id, target_id, std::move(headline));
}

std::map<uint64_t, Card> database::search_cards(uint64_t const subject_id, expertise_level const level, std::string_view search_pattern) const
{
    std::map<uint64_t, Card> matched{};

    if (!search_pattern.empty())
    {
        for (pqxx::row const& row: query("select similarity, id, state, headline from search_cards($1, $2, $3) order by similarity", subject_id, level_to_string(level),
                                         search_pattern))
        {
            uint64_t const similarity{row.at("similarity").as<uint64_t>()};
            Card card{};
            card.set_id(row.at("id").as<uint64_t>());
            card.set_headline(row.at("headline").as<std::string>());
            matched.insert({similarity, card});
        }
    }

    return matched;
}

void database::move_card_to_section(uint64_t const card_id, uint64_t const resource_id, uint64_t const section_position, uint64_t const target_section_position) const
{
    exec("call move_card_to_section($1, $2, $3, $4)", card_id, resource_id, section_position, target_section_position);
}

void database::move_card_to_topic(uint64_t const card_id, uint64_t const subject_id, uint64_t const topic_position, expertise_level const topic_level,
                                  uint64_t const target_subject, uint64_t const target_position, expertise_level const target_level) const
{
    exec("call move_card_to_topic($1, $2, $3, $4, $5, $6, $7)", card_id, subject_id, topic_position, level_to_string(topic_level), target_subject, target_position,
         level_to_string(target_level));
}

std::vector<Card> database::get_section_cards(uint64_t const resource_id, uint64_t const sections_position) const
{
    std::vector<Card> cards{};

    for (pqxx::result const result{query("select id, state, headline from get_section_cards($1, $2)", resource_id, sections_position)}; pqxx::row const& row: result)
    {
        Card card{};
        card.set_id(row.at("id").as<uint64_t>());
        card.set_state(to_card_state(row.at("state").as<std::string>()));
        card.set_headline(row.at("headline").as<std::string>());
        cards.push_back(card);
    }

    return cards;
}

std::vector<Card> database::get_topic_cards(uint64_t const subject_id, uint64_t const topic_position, expertise_level const topic_level) const
{
    std::vector<Card> cards{};

    for (pqxx::result const result{query("select id, state, headline from get_topic_cards($1, $2, $3)", subject_id, topic_position, level_to_string(topic_level))};
         pqxx::row const& row: result)
    {
        Card card{};
        card.set_id(row.at("id").as<uint64_t>());
        card.set_state(to_card_state(row.at("state").as<std::string>()));
        card.set_headline(row.at("headline").as<std::string>());
        cards.push_back(card);
    }

    return cards;
}

Block database::create_block(uint64_t const card_id, Block block) const
{
    if (!block.extension().empty() && !block.content().empty())
    {
        if (block.position() > 0)
        {
            exec("call create_block($1, $2, $3, $4, $5, $6)", card_id, content_type_to_string(block.type()), block.extension(), block.content(), block.metadata(),
                 block.position());
        }
        else
        {
            if (pqxx::result const result{
                query("select create_block($1, $2, $3, $4, $5) as id", card_id, content_type_to_string(block.type()), block.extension(), block.content(), block.metadata())
            }; result.size() == 1)
            {
                block.set_position(result.at(0).at("id").as<uint64_t>());
            }
        }
    }
    return block;
}

std::map<uint64_t, Block> database::get_blocks(uint64_t const card_id) const
{
    std::map<uint64_t, Block> blocks{};

    for (pqxx::result const result{query("select position, type, extension, metadata, content from get_blocks($1)", card_id)}; pqxx::row const& row: result)
    {
        Block block{};
        uint64_t const position{row.at("position").as<uint64_t>()};
        block.set_position(position);
        block.set_type(to_content_type(row.at("type").as<std::string>()));
        block.set_extension(row.at("extension").as<std::string>());
        block.set_metadata(row.at("metadata").is_null() ? "" : row.at("metadata").as<std::string>());
        block.set_content(row.at("content").as<std::string>());
        blocks.insert({position, block});
    }

    return blocks;
}

void database::remove_block(uint64_t const card_id, uint64_t const block_position) const
{
    exec("call remove_block($1, $2)", card_id, block_position);
}

void database::edit_block_content(uint64_t const card_id, uint64_t const block_position, std::string content) const
{
    exec("call edit_block_content($1, $2, $3)", card_id, block_position, content);
}

void database::change_block_type(uint64_t const card_id, uint64_t const block_position, Block::content_type const type) const
{
    exec("call change_block_type($1, $2, $3)", card_id, block_position, content_type_to_string(type));
}

void database::edit_block_extension(uint64_t const card_id, uint64_t const block_position, std::string extension) const
{
    exec("call edit_block_extension($1, $2, $3)", card_id, block_position, extension);
}

void database::edit_block_metadata(uint64_t const card_id, uint64_t const block_position, std::string metadata) const
{
    exec("call edit_block_metadata($1, $2, $3)", card_id, block_position, metadata);
}

void database::reorder_block(uint64_t const card_id, uint64_t const block_position, uint64_t const target_position) const
{
    exec("call reorder_block($1, $2, $3)", card_id, block_position, target_position);
}

void database::merge_blocks(uint64_t const card_id, uint64_t const source_position, uint64_t const target_position) const
{
    exec("call merge_blocks($1, $2, $3)", card_id, source_position, target_position);
}

std::map<uint64_t, Block> database::split_block(uint64_t const card_id, uint64_t const block_position) const
{
    std::map<uint64_t, Block> blocks{};
    pqxx::result const result{query("select position, type, extension, metadata, content from split_block($1, $2)", card_id, block_position)};
    int const rows{result.size()};

    for (pqxx::row const& row: result)
    {
        Block block{};
        block.set_position(row.at("position").as<uint64_t>());
        block.set_type(to_content_type(row.at("type").as<std::string>()));
        block.set_extension(row.at("extension").as<std::string>());
        block.set_metadata(row.at("metadata").is_null() ? "" : row.at("metadata").as<std::string>());
        block.set_content(row.at("content").as<std::string>());
        blocks.insert({block.position(), block});
    }

    return blocks;
}

void database::move_block(uint64_t const card_id, uint64_t const block_position, uint64_t const target_card_id, uint64_t const target_position) const
{
    exec("call move_block($1, $2, $3, $4)", card_id, block_position, target_card_id, target_position);
}

Resource database::create_nerve(uint64_t const user_id, std::string resource_name, uint64_t const subject_id, uint64_t const expiration) const
{
    Resource resource{};
    resource.clear_id();
    resource.set_name(resource_name);

    if (pqxx::result const result{query("select create_nerve($1, $2, $3, $4) as id", user_id, std::move(resource_name), subject_id, expiration)}; result.size() == 1)
    {
        resource.set_id(result.at(0).at("id").as<uint64_t>());
    }

    return resource;
}

std::vector<Resource> database::get_nerves(uint64_t const user_id) const
{
    std::vector<Resource> nerves{};

    for (pqxx::row const& row: query("select id, name, type, pattern, link, production, expiration from get_nerves($1)", user_id))
    {
        Resource resource{};
        resource.set_id(row.at("id").as<uint64_t>());
        resource.set_name(row.at("name").as<std::string>());
        resource.set_type(to_resource_type(row.at("type").as<std::string>()));
        resource.set_pattern(to_section_pattern(row.at("pattern").as<std::string>()));
        resource.set_link(row.at("link").is_null() ? "" : row.at("link").as<std::string>());
        resource.set_production(row.at("production").as<uint64_t>());
        resource.set_expiration(row.at("expiration").as<uint64_t>());
        nerves.push_back(resource);
    }

    return nerves;
}

expertise_level database::get_user_cognitive_level(uint64_t const user_id, uint64_t const subject_id) const
{
    auto level{expertise_level::surface};

    for (pqxx::result const result{query("select get_user_cognitive_level($1, $2)", user_id, subject_id)}; pqxx::row row: result)
    {
        level = to_level(row.at(0).as<std::string>());
    }

    return level;
}

practice_mode database::get_practice_mode(uint64_t const user_id, uint64_t const subject_id, expertise_level const level) const
{
    return {};
}

std::map<uint64_t, Topic> database::get_practice_topics(uint64_t const user_id, uint64_t const subject_id) const
{
    return {};
}

std::vector<Card> database::get_practice_cards(uint64_t const user_id, uint64_t const subject_id, uint64_t const topic_position) const
{
    return {};
}

std::vector<Resource> database::get_study_resources(uint64_t const user_id) const
{
    return {};
}

std::map<uint64_t, Section> database::get_study_sections(uint64_t const user_id, uint64_t const resource_id) const
{
    return {};
}

std::map<uint64_t, Card> database::get_study_cards(uint64_t const user_id, uint64_t const resource_id, uint64_t const section_position) const
{
    return {};
}

void database::mark_card_as_reviewed(uint64_t const card_id) const
{
}

void database::mark_card_as_completed(uint64_t const card_id) const
{
}

void database::mark_section_as_reviewed(uint64_t const resource_id, uint64_t const section_position) const
{
}

void database::mark_section_as_completed(uint64_t const resource_id, uint64_t const section_position) const
{
}

void database::mark_card_as_approved(uint64_t const card_id) const
{
}

void database::mark_card_as_released(uint64_t const card_id) const
{
}

void database::make_progress(uint64_t const user_id, uint64_t const card_id, uint64_t const duration, practice_mode const mode) const
{
}

closure_state database::get_section_state(uint64_t const resource_id, uint64_t const section_position) const
{
    return {};
}

closure_state database::get_resource_state(uint64_t const resource_id) const
{
    return {};
}

Weight database::get_progress_weight(uint64_t const user_id) const
{
    return {};
}

std::vector<Card> database::get_variations(uint64_t const card_id) const
{
    return {};
}

bool database::is_absolute(uint64_t const card_id) const
{
    return {};
}

void database::create_assessment(uint64_t const subject_id, expertise_level const level, uint64_t const topic_position, uint64_t const card_id) const
{
}

void database::get_topic_coverage(uint64_t const assessment_id) const
{
}

void database::get_assessment_coverage(uint64_t const subject_id, uint64_t const topic_position, expertise_level const max_level) const
{
}

void database::get_assimilation_coverage(uint64_t const user_id, uint64_t const assessment_id) const
{
}

std::vector<Card> database::get_topic_assessments(uint64_t const user_id, uint64_t const subject_id, uint64_t const topic_position, expertise_level const max_level) const
{
    return {};
}

std::vector<Card> database::get_assessments(uint64_t const user_id, uint64_t const subject_id, uint64_t const topic_position) const
{
    return {};
}

void database::expand_assessment(uint64_t const assessment_id, uint64_t const subject_id, expertise_level const level, uint64_t const topic_position) const
{
}

void database::diminish_assessment(uint64_t const assessment_id, uint64_t const subject_id, expertise_level const level, uint64_t const topic_position) const
{
}
