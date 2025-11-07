#include <iostream>
#include <vector>
#include <string>
#include <format>
#include <memory>
#include <boost/asio.hpp>
#include <flashback/exceptions.hpp>
#include <flashback/options.hpp>
#include <flashback/event_handler.hpp>

int main(int const argc, char** argv)
{
    try
    {
        std::vector<std::string> const args(argv + 1, argv + argc);
        auto options{std::make_shared<flashback::options>(args)};
        auto event_handler{std::make_unique<flashback::event_handler>(options)};
    }
    catch (flashback::descriptive_option const& opt)
    {
        std::cerr << std::format("{}", opt.what());
    }
    catch (boost::system::system_error const& err)
    {
        std::cerr << std::format("{}", err.code().message());
        return err.code().value();
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("error: {}", exp.what());
        return 1;
    }
}
