#include <flashback/roadmap_page.hpp>
#include <thread>
#include <chrono>

using namespace flashback;

roadmap_page::roadmap_page(std::shared_ptr<client> client)
    : m_client{client}
{
    std::this_thread::sleep_for(std::chrono::seconds(2));
    add_roadmaps();
    page::display(ftxui::hbox(ftxui::vbox(m_elements), ftxui::flex));
}

void roadmap_page::add_roadmaps()
{
    std::ranges::for_each(m_client->get_roadmaps()->roadmap(), std::bind_front(&roadmap_page::add_roadmap, this));
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
