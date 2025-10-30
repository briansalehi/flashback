#include <flashback/welcome.hpp>

using namespace flashback;

welcome::welcome(std::shared_ptr<ftxui::Screen> screen)
    : m_screen{screen}
{
}
