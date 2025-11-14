#include <algorithm>
#include <memory>
#include <utility>
#include <iostream>
#include <format>
#include <flashback/welcome_page.hpp>

using namespace flashback;

welcome_page::welcome_page(std::shared_ptr<client> client)
    : m_client{client}
{
    std::shared_ptr<User> current_user{std::make_shared<User>()};
    current_user->set_id(2);
    add_roadmaps(m_client->get_roadmaps(current_user));
    display();
}

void welcome_page::add_roadmaps(std::shared_ptr<Roadmaps> input)
{
    std::ranges::for_each(input->roadmaps(), std::bind_front(&welcome_page::add_roadmap, this));
}

void welcome_page::add_roadmap(Roadmap const& r)
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

void welcome_page::display()
{
    content(ftxui::hbox(ftxui::vbox(m_elements, ftxui::flex), ftxui::flex));
    page::display();
}
