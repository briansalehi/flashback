#pragma once

#include <memory>
#include <set>
#include <ftxui/dom/elements.hpp>
#include <flashback/server.hpp>
#include <flashback/page.hpp>
#include <flashback/types.hpp>

namespace flashback
{
class welcome final: public page
{
public:
    explicit welcome(std::shared_ptr<server> server);
    ~welcome() override = default;

    void add_roadmaps(std::set<roadmap> const& roadmaps);
    void add_roadmap(roadmap const& r);

protected:
    void display() override;

private:
    ftxui::Elements m_elements;
    std::shared_ptr<server> m_server;
};
} // namespace flashback
