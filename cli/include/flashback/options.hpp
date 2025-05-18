#pragma once

#include <boost/program_options.hpp>
#include <boost/asio.hpp>
#include <string>
#include <vector>

namespace flashback
{
class options
{
public:
    explicit options(std::vector<std::string> const& args);
public:
    std::string server_address;
    std::string server_port;
private:
    boost::program_options::options_description opts;
    boost::program_options::variables_map vmap;
};
} // flashback