#include <flashback/options.hpp>
#include <flashback/exceptions.hpp>
#include <boost/system.hpp>
#include <iostream>
#include <sstream>
#include <numeric>
#include <format>

using namespace flashback;

constexpr auto server_domain_name{"https://flashback.eu.com"};
constexpr auto server_port_number{"9821"};

options::options(std::vector<std::string> const& args)
    : opts{"Options"}
{
    auto const default_address{boost::program_options::value<std::string>()->default_value(server_domain_name)};
    auto const default_port{boost::program_options::value<std::string>()->default_value(server_port_number)};
    boost::program_options::positional_options_description address{};
    address.add("address", 1).add("port", 2);

    opts.add_options()
        ("help,h", "show help menu")
        ("version,v", "program version")
        ("address,a", default_address, "server address")
        ("port,p", default_port, "server port");

    try
    {
        boost::program_options::command_line_parser parser{args};
        boost::program_options::parsed_options parsed_options{parser.options(opts).positional(address).run()};
        boost::program_options::store(parsed_options, vmap);
        vmap.notify();
    }
    catch (boost::system::system_error const& exp)
    {
        std::cerr << std::format("{}", exp.what());
    }

    if (vmap.contains("help"))
    {
        std::ostringstream stream;
        stream << opts;
        throw descriptive_option{stream.str()};
    }

    if (vmap.contains("version"))
    {
        throw descriptive_option{PROGRAM_VERSION};
    }

    server_address = vmap["address"].as<std::string>();
    server_port = vmap["port"].as<std::string>();
}