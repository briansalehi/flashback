#include <flashback/exceptions.hpp>
#include <flashback/options.hpp>
#include <flashback/client.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <print>

int main(int const argc, char** argv)
{
    try
    {
        std::vector<std::string> const args(argv + 1, argv + argc);
        flashback::options const options(args);
        flashback::client client{options};
    }
    catch (flashback::descriptive_option const& opt)
    {
        std::println(std::cerr, "{}", opt.what());
    }
    catch (boost::system::system_error const& err)
    {
        std::println(std::cerr, "{}", err.code().message());
        return err.code().value();
    }
    catch (std::exception const& exp)
    {
        std::println(std::cerr, "error: {}", exp.what());
        return 1;
    }
}