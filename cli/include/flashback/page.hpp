#pragma once

#include <ftxui/dom/elements.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>

namespace flashback
{
class page
{
public:
    explicit page();
    virtual ~page() = default;

    void render();
    void close();

protected:
    virtual void display(ftxui::Element element);
    virtual void heading(ftxui::Element element);

private:
    [[nodiscard]] bool onEvent(ftxui::Event const& event);

    ftxui::Element m_heading;
    ftxui::Element m_content;
    ftxui::ScreenInteractive m_screen;
    ftxui::Component m_renderer;
};
} // flashback
