#pragma once

namespace flashback
{
class page
{
public:
    virtual void display() const noexcept = 0;
};
} // flashback