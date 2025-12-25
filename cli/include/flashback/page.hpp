#pragma once

#include <memory>
#include <utility>
#include <functional>
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
    virtual std::pair<ftxui::Component, std::function<ftxui::Element()>> prepare_components() = 0;
    void render();
    void close();

protected:
    virtual void display(std::function<ftxui::Element()> const&  element);
    virtual void display(ftxui::Component component, std::function<ftxui::Element()> const&  element);
    virtual void headline(ftxui::Element element);

private:
    [[nodiscard]] bool handle_event(ftxui::Event const& event);

    ftxui::Element m_headline;
    std::function<ftxui::Element()> m_content;
    ftxui::Component m_component;
    ftxui::ScreenInteractive m_screen;
    ftxui::Component m_renderer;
};
} // flashback
