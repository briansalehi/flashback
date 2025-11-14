#pragma once

#include <memory>
#include <ftxui/dom/elements.hpp>
#include <flashback/client.hpp>
#include <flashback/page.hpp>
#include <types.pb.h>

namespace flashback
{
class welcome_page final: public page
{
public:
    explicit welcome_page(std::shared_ptr<client> server);
    ~welcome_page() override = default;

    void add_roadmaps(std::shared_ptr<Roadmaps> roadmaps);
    void add_roadmap(Roadmap const& r);

protected:
    void display() override;

private:
    ftxui::Elements m_elements;
    std::shared_ptr<client> m_client;
};
} // namespace flashback
