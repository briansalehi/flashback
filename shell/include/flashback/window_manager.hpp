#pragma once

#include <memory>
#include <thread>
#include <mutex>
#include <flashback/page.hpp>
#include <flashback/client.hpp>

namespace flashback
{
class window_manager: public std::enable_shared_from_this<window_manager>
{
public:
    explicit window_manager(std::shared_ptr<client> client);
    ~window_manager();
    void start();

    void display_signup();
    void display_signin();
    void display_roadmaps();

private:
    void display();
    void swap_page(std::shared_ptr<page> next_page);

    std::shared_ptr<client> m_client;
    std::shared_ptr<page> m_page;
    std::shared_ptr<page> m_next_page;
    std::unique_ptr<std::mutex> m_window_lock;
    std::unique_ptr<std::jthread> m_window_runner;
};
} // flashback
