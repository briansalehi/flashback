#pragma once

#include <flashback/page.hpp>
#include <flashback/client.hpp>

namespace flashback
{
class roadmap_page: public page
{
public:
    explicit roadmap_page(std::shared_ptr<client> client);
    ~roadmap_page() override = default;

    void add_roadmaps();
    void add_roadmap(Roadmap const& r);

private:
    ftxui::Elements m_elements;
    std::shared_ptr<client> m_client;
};
} // flashback
