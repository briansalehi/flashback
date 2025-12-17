#include <flashback/page.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/dom/elements.hpp>

flashback::page::page()
    : m_headline{
        ftxui::vbox(
            ftxui::paragraphAlignCenter("Flashback") | ftxui::color(ftxui::Color::Red) | ftxui::bold,
            ftxui::separator() | ftxui::color(ftxui::Color::GrayDark)
        )
    }
    , m_screen{ftxui::ScreenInteractive::Fullscreen()}
{
}

void flashback::page::display(std::function<ftxui::Element()> const& content)
{
    m_content = content;
}

void flashback::page::display(ftxui::Component component, std::function<ftxui::Element()> const& content)
{
    m_component = component;
    m_content = content;
}

void flashback::page::headline(ftxui::Element element)
{
    m_headline = std::move(element);
}

bool flashback::page::handle_event(ftxui::Event const& event)
{
    if (event == ftxui::Event::Escape)
    {
        m_screen.Exit();
        return true;
    }
    return false;
}

void flashback::page::render()
{
    if (m_component == nullptr)
    {
        m_renderer = ftxui::Renderer(m_content);
    }
    else
    {
        m_renderer = ftxui::Renderer(m_component, m_content);
    }
    m_renderer = ftxui::CatchEvent(m_renderer, std::bind_front(&page::handle_event, this));
    m_screen.Loop(m_renderer);
}

void flashback::page::close()
{
    m_screen.Exit();
}
