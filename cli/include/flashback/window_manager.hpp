#pragma once

#include <memory>
#include <thread>
#include <mutex>
#include <flashback/page.hpp>
#include <flashback/client.hpp>

namespace flashback
{
class window_manager
{
public:
    explicit window_manager(std::shared_ptr<client> client);
    ~window_manager() = default;

protected:
    void display();

private:
    std::shared_ptr<client> m_client;
    std::shared_ptr<page> m_page;
    std::unique_ptr<std::mutex> m_window_lock;
    std::unique_ptr<std::jthread> m_window_runner;
};
} // flashback
