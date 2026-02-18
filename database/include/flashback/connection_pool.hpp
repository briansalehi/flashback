#pragma once

#include <pqxx/pqxx>
#include <memory>
#include <string>
#include <mutex>
#include <condition_variable>
#include <queue>

namespace flashback
{
class connection_pool
{
public:
    connection_pool(std::string connection_string, size_t pool_size = 10);
    ~connection_pool() = default;

    // RAII wrapper for automatic connection return
    class connection_guard
    {
    public:
        connection_guard(connection_pool* pool, std::unique_ptr<pqxx::connection> conn)
            : m_pool{pool}, m_connection{std::move(conn)}
        {}

        ~connection_guard()
        {
            if (m_connection && m_pool)
            {
                m_pool->return_connection(std::move(m_connection));
            }
        }

        // Prevent copying
        connection_guard(connection_guard const&) = delete;
        connection_guard& operator=(connection_guard const&) = delete;

        // Allow moving
        connection_guard(connection_guard&& other) noexcept
            : m_pool{other.m_pool}, m_connection{std::move(other.m_connection)}
        {
            other.m_pool = nullptr;
        }

        connection_guard& operator=(connection_guard&& other) noexcept
        {
            if (this != &other)
            {
                m_pool = other.m_pool;
                m_connection = std::move(other.m_connection);
                other.m_pool = nullptr;
            }
            return *this;
        }

        pqxx::connection& operator*() { return *m_connection; }
        pqxx::connection* operator->() { return m_connection.get(); }
        pqxx::connection* get() { return m_connection.get(); }

    private:
        connection_pool* m_pool;
        std::unique_ptr<pqxx::connection> m_connection;
    };

    // Acquire a connection from the pool
    [[nodiscard]] connection_guard acquire();

private:
    void return_connection(std::unique_ptr<pqxx::connection> conn);

    std::string m_connection_string;
    size_t m_pool_size;
    std::queue<std::unique_ptr<pqxx::connection>> m_available_connections;
    std::mutex m_mutex;
    std::condition_variable m_condition;
};
} // namespace flashback
