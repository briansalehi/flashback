#include <algorithm>
#include <memory>
#include <utility>
#include <iostream>
#include <format>
#include <flashback/welcome.hpp>

constexpr std::chrono::milliseconds refresh_rate{1000};

using namespace flashback;

welcome::welcome(std::shared_ptr<client> client)
    : m_client{client}
{
    std::shared_ptr<user> current_user{std::make_shared<user>()};
    current_user->set_id(2);
    add_roadmaps(m_client->get_roadmaps(current_user));
    display();
}

void welcome::add_roadmaps(std::shared_ptr<roadmaps> input)
{
    std::ranges::for_each(input->roadmap(), std::bind_front(&welcome::add_roadmap, this));
    for (roadmap const& r: input->roadmap())
    {
        std::clog << std::format("Welcome Page: resource ({}) {}\n", r.id(), r.name());
    }
}

void welcome::add_roadmap(roadmap const& r)
{
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

void welcome::display()
{
    content(ftxui::hbox(ftxui::vbox(m_elements, ftxui::flex), ftxui::flex));
    page::display();
}
