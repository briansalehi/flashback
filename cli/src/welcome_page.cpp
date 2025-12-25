#include <memory>
#include <functional>
#include <iostream>
#include <flashback/welcome_page.hpp>

using namespace flashback;

welcome_page::welcome_page()
{
    try
    {
        ftxui::Element title{ftxui::paragraphAlignCenter("LOADING") | ftxui::blink | ftxui::dim | ftxui::bold};
        ftxui::Component component{};
        std::function<ftxui::Element()> content{
            [title] {
                return ftxui::vbox(ftxui::filler(), title, ftxui::filler());
            }
        };

        page::display(content);
    }
    catch (std::runtime_error const& exp)
    {
        std::cerr << "Client: " << exp.what() << std::endl;
    }
}

std::pair<ftxui::Component, std::function<ftxui::Element()>> welcome_page::prepare_components()
{
    return {};
}
