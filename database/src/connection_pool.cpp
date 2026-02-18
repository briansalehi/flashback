#include <flashback/connection_pool.hpp>
#include <iostream>

using namespace flashback;

connection_pool::connection_pool(std::string connection_string, size_t pool_size)
    : m_connection_string{std::move(connection_string)}, m_pool_size{pool_size}
{
    // Pre-create connections in the pool
    for (size_t i = 0; i < m_pool_size; ++i)
    {
        try
        {
            m_available_connections.push(std::make_unique<pqxx::connection>(m_connection_string));
        }
        catch (std::exception const& exp)
        {
            std::cerr << "Failed to create connection in pool: " << exp.what() << std::endl;
            throw;
        }
    }
}

connection_pool::connection_guard connection_pool::acquire()
{
    std::unique_lock<std::mutex> lock{m_mutex};

    // Wait until a connection is available
    m_condition.wait(lock, [this] { return !m_available_connections.empty(); });

    // Get a connection from the pool
    auto conn = std::move(m_available_connections.front());
    m_available_connections.pop();

    return connection_guard{this, std::move(conn)};
}

void connection_pool::return_connection(std::unique_ptr<pqxx::connection> conn)
{
    std::lock_guard<std::mutex> lock{m_mutex};
    m_available_connections.push(std::move(conn));
    m_condition.notify_one();
}
