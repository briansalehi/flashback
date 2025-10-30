#pragma once

#include <ftxui/screen/screen.hpp>
#include <memory>

namespace flashback
{

class welcome
{
public:
    explicit welcome(std::shared_ptr<ftxui::Screen> screen);

private:
    std::shared_ptr<ftxui::Screen> m_screen;
};

} // namespace flashback
