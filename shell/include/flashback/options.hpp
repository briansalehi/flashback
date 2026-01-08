#pragma once

#include <boost/program_options.hpp>
#include <string>
#include <vector>

namespace flashback
{
class options
{
public:
    std::string server_address;
    std::string server_port;

    explicit options(std::vector<std::string> const& args);

private:
    boost::program_options::options_description opts;
    boost::program_options::variables_map vmap;
};
} // flashback
