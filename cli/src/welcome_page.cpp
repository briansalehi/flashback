#include <memory>
#include <flashback/welcome_page.hpp>

using namespace flashback;

welcome_page::welcome_page()
{
    try
    {
        ftxui::Element title{ftxui::paragraphAlignCenter("LOADING") | ftxui::blink | ftxui::dim | ftxui::bold};
        ftxui::Element content{ftxui::vbox(ftxui::flex, title)};
        page::display(content);
    }
    catch (std::runtime_error const& exp)
    {
        std::cerr << "Client: " << exp.what() << std::endl;
    }
}
