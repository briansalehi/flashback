#include <flashback/page.hpp>

namespace flashback
{
class roadmap_page : public page
{
    void display() const noexcept override;
};
} // flashback