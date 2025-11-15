#include <flashback/roadmap_page.hpp>

using namespace flashback;

roadmap_page::roadmap_page(std::shared_ptr<client> client)
    : m_client{client}
{
    add_roadmaps();
}

void roadmap_page::add_roadmaps()
{
    std::ranges::for_each(m_client->get_roadmaps()->roadmaps(), std::bind_front(&roadmap_page::add_roadmap, this));
}

void roadmap_page::add_roadmap(Roadmap const& r)
{
    std::clog << std::format("Welcome Page: resource ({}) {}\n", r.id(), r.name());
    ftxui::Element element{
        ftxui::border(
            ftxui::hbox(
                ftxui::text(std::to_string(r.id())),
                ftxui::separatorEmpty(),
                ftxui::text(r.name())
                )
            )
    };

    m_elements.push_back(std::move(element));
}

void roadmap_page::display()
{
    content(ftxui::hbox(ftxui::vbox(m_elements, ftxui::flex), ftxui::flex));
    page::display();
}
