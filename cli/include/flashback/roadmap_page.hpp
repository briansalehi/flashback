#pragma once

#include <flashback/page.hpp>
#include <flashback/client.hpp>

namespace flashback
{
class window_manager;

class roadmap_page final: public page
{
public:
    explicit roadmap_page(std::shared_ptr<client> client, std::weak_ptr<window_manager> window);
    ~roadmap_page() override = default;
    std::pair<ftxui::Component, std::function<ftxui::Element()>> prepare_components() override;

private:
    void load_roadmaps();
    void add_roadmap(Roadmap const& r);

    ftxui::Elements m_elements;
    std::shared_ptr<client> m_client;
    std::weak_ptr<window_manager> m_window_manager;
    std::vector<Roadmap> m_roadmaps;
};
} // flashback
