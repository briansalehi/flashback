#include <algorithm>
#include <functional>
#include <utility>
#include <flashback/welcome.hpp>

constexpr std::chrono::milliseconds refresh_rate{1000};

using namespace flashback;

welcome::welcome(std::shared_ptr<server> server)
    : m_server{server}
{
    add_roadmaps(server->roadmaps(2));
    display();
}

void welcome::add_roadmaps(std::set<roadmap> const& roadmaps)
{
    std::ranges::for_each(roadmaps, std::bind_front(&welcome::add_roadmap, this));
}

void welcome::add_roadmap(roadmap const& r)
{
    ftxui::Element element{
        ftxui::border(
            ftxui::hbox(
                ftxui::text(std::to_string(r.id)),
                ftxui::separatorEmpty(),
                ftxui::text(r.name)
            )
        )
    };

    m_elements.push_back(std::move(element));
}

void welcome::display()
{
    content(ftxui::hbox(ftxui::vbox(m_elements, ftxui::flex), ftxui::flex));
    page::display();
}
