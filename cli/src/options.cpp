#include <flashback/options.hpp>
#include <flashback/exceptions.hpp>
#include <iostream>
#include <sstream>
#include <numeric>
#include <print>

using namespace flashback;

constexpr auto server_domain_name{"https://flashback.training"};
constexpr auto server_port_number{"9821"};

options::options(std::vector<std::string> const& args)
    : opts{"Options"}
{
    auto const default_address{boost::program_options::value<std::string>()->default_value(server_domain_name)};
    auto const default_port{boost::program_options::value<std::string>()->default_value(server_port_number)};

    opts.add_options()
        ("help,h", "show help menu")
        ("version,v", "program version")
        ("address,a", default_address, "server address")
        ("port,p", default_port, "server port");

    try
    {
        boost::program_options::command_line_parser parser{boost::program_options::command_line_parser(args)};
        boost::program_options::parsed_options parsed_options{parser.options(opts).run()};
        boost::program_options::store(parsed_options, vmap);
        vmap.notify();
    }
    catch (boost::system::system_error const& exp)
    {
        std::println(std::cerr, "{}", exp.what());
    }

    if (vmap.contains("help"))
    {
        std::ostringstream stream;
        stream << opts;
        throw flashback::descriptive_option{stream.str()};
    }

    if (vmap.contains("version"))
    {
        throw flashback::descriptive_option{PROGRAM_VERSION};
    }

    server_address = vmap["address"].as<std::string>();
    server_port = vmap["port"].as<std::string>();
}