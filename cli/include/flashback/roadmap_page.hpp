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
    std::pair<ftxui::Component, std::function<ftxui::Element()>> prepare_components() override;

private:
    void load_roadmaps();
    void add_roadmap(Roadmap const& r);

    ftxui::Elements m_elements;
    std::shared_ptr<client> m_client;
    std::vector<Roadmap> m_roadmaps;
};
} // flashback
