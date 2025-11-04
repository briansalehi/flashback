#pragma once

#include <memory>
#include <ftxui/dom/elements.hpp>
#include <flashback/client.hpp>
#include <flashback/page.hpp>
#include <types.pb.h>

namespace flashback
{
class welcome final: public page
{
public:
    explicit welcome(std::shared_ptr<client> server);
    ~welcome() override = default;

    void add_roadmaps(std::shared_ptr<roadmaps> roadmaps);
    void add_roadmap(roadmap const& r);

protected:
    void display() override;

private:
    ftxui::Elements m_elements;
    std::shared_ptr<client> m_client;
};
} // namespace flashback
