#include <flashback/config_manager.hpp>

using namespace flashback;

config_manager::config_manager()
    : m_loaded_user{nullptr}
{
}

std::filesystem::path config_manager::base_path() const noexcept
{
    return m_config_path;
}

std::filesystem::path config_manager::config_path() const noexcept
{
    return m_config_path;
}

void config_manager::base_path(std::filesystem::path path)
{
    m_base_path = path;
    m_config_path = path / std::filesystem::path("flashback.conf");
}

void config_manager::reload()
{
    m_config.open(m_config_path);
    if (!std::filesystem::exists(m_config_path))
    {
        std::filesystem::create_directory(m_config_path);
    }
}

std::shared_ptr<User> config_manager::get_user() const
{
    return m_loaded_user;
}

void config_manager::load_user()
{
    if (std::filesystem::exists(m_config_path))
    {
    }
    else
    {
        throw std::runtime_error("config file does not exist");
    }
}
