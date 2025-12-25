#pragma once

#include <ftxui/dom/elements.hpp>
#include <flashback/page.hpp>

namespace flashback
{
class welcome_page final: public page
{
public:
    explicit welcome_page();
    ~welcome_page() override = default;
    std::pair<ftxui::Component, std::function<ftxui::Element()>> prepare_components() override;

private:
    ftxui::Elements m_elements;
};
} // namespace flashback
