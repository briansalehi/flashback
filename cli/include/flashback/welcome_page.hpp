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

private:
    ftxui::Elements m_elements;
};
} // namespace flashback
