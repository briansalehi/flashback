#include <functional>
#include <flashback/roadmap_page.hpp>

using namespace flashback;

roadmap_page::roadmap_page(std::shared_ptr<client> client)
    : m_client{client}
{
    load_roadmaps();
    std::function<ftxui::Element()> const content{[this] { return ftxui::hbox(ftxui::vbox(m_elements), ftxui::flex); }};
    page::display(content);
}

std::pair<ftxui::Component, std::function<ftxui::Element()>> roadmap_page::prepare_components()
{
    return {};
}

void roadmap_page::add_roadmap(Roadmap const& r)
{
    ftxui::Element element{ftxui::border(ftxui::hbox(ftxui::text(std::to_string(r.id())), ftxui::separatorEmpty(), ftxui::text(r.name())))};
    m_elements.push_back(std::move(element));
}

void roadmap_page::load_roadmaps()
{
    std::ranges::for_each(m_client->get_roadmaps(), std::bind_front(&roadmap_page::add_roadmap, this));
}
