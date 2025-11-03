#include <flashback/page.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/dom/elements.hpp>

flashback::page::page()
    : m_heading{
        ftxui::vbox(
            ftxui::paragraphAlignCenter("Flashback") | ftxui::color(ftxui::Color::Red) | ftxui::bold,
            ftxui::separator() | ftxui::color(ftxui::Color::GrayDark)
        )
    }
    , m_screen{ftxui::ScreenInteractive::Fullscreen()}
{
}

void flashback::page::display()
{
    m_renderer = ftxui::Renderer([this] -> ftxui::Element { return m_content; });
    m_renderer = ftxui::CatchEvent(m_renderer, std::bind_front(&page::onEvent, this));
    m_screen.Loop(m_renderer);
}

void flashback::page::heading(ftxui::Element element)
{
    m_heading = std::move(element);
}

void flashback::page::content(ftxui::Element element)
{
    m_content = ftxui::vbox(m_heading, ftxui::hbox(element, ftxui::flex), ftxui::flex);
}

bool flashback::page::onEvent(ftxui::Event const& event)
{
    if (event == ftxui::Event::Escape)
    {
        m_screen.ExitLoopClosure()();
        return true;
    }
    return false;
}
