#include <flashback/server.hpp>

using namespace flashback;

server::server(std::shared_ptr<options> options) :
    m_context{std::make_unique<boost::asio::io_context>()}
    , m_remote{std::make_unique<boost::asio::ip::tcp::socket>(*m_context)}
    , m_address{options->server_address}
    , m_port{options->server_port}
{
}

void server::connect()
{
    boost::asio::ip::tcp::resolver resolver{*m_context};
    boost::asio::ip::tcp::resolver::query const query{m_address, m_port};
    boost::asio::ip::tcp::resolver::iterator candidate{resolver.resolve(query)};
    std::vector<boost::asio::ip::tcp::endpoint> candidates(candidate, {});
}

std::set<roadmap> server::roadmaps(std::uint64_t user) const
{
    return {
        {1, "Embedded Linux Software"},
        {2, "String Theory Physics"}
    };
}

std::set<milestone> server::milestones(std::uint64_t roadmap) const
{
    return {
        {1, 6, "C++", expertise_level::surface},
        {2, 3, "Boost", expertise_level::surface}
    };
}

std::set<resource> server::resources(std::uint64_t user, std::uint64_t subject) const
{
    return {
        {
            27, "C++17 STL Cookbook", "Jacek Galowicz", "Packt Publishing", "", resource_type::book,
            section_pattern::chapter, resource_condition::relevant, {}
        },
        {
            19, "C++20 STL Cookbook", "Bill Weinman", "Packt Publishing", "", resource_type::book,
            section_pattern::chapter, resource_condition::relevant, {}
        }
    };
}

std::set<section> server::sections(std::uint64_t resource) const
{
    return {
        {1, "Strategy"},
        {2, "Template Method"},
        {3, "Command"},
        {4, "Memento"}
    };
}

std::set<topic> server::topics(std::uint64_t roadmap, std::uint64_t milestone) const
{
    return {
        {1, "Building Executables"},
        {2, "Fundamental Data Types"},
        {3, "Variable Initialization"},
        {4, "Constant Initialization"}
    };
}

std::map<std::uint64_t, card> server::study_cards(std::uint64_t resource, std::uint64_t section) const
{
    return {
        {1, {116, card_state::review, "How many constants are available in C++?"}},
        {2, {117, card_state::review, "Initialize a constant?"}}
    };
}

std::map<std::uint64_t, card> server::practice_cards(std::uint64_t roadmap, std::uint64_t milestone,
                                                     std::uint64_t topic) const
{
    return {
        {1, {116, card_state::review, "How many constants are available in C++?"}},
        {2, {117, card_state::review, "Initialize a constant?"}}
    };
}
