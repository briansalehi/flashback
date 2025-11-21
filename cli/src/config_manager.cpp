#include <random>
#include <format>
#include <sstream>
#include <flashback/config_manager.hpp>

using namespace flashback;

config_manager::config_manager()
    : m_loaded_user{nullptr}
    , m_base_path{"/home/hsalehipour/.config/flashback"}
    , m_config_path{m_base_path / "flashback.conf"}
{
    config_manager::make_base();
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
    m_config_path = path / "flashback.conf";
}

void config_manager::store(std::shared_ptr<User> user)
{
    m_config.open(m_config_path, std::ios::out);

    if (!m_config.is_open())
    {
        throw std::runtime_error("Config: configuration cannot be written to filesystem");
    }

    m_config << std::format("id={}\n", user->id());
    m_config << std::format("name={}\n", user->name());
    m_config << std::format("email={}\n", user->email());
    m_config << std::format("token={}\n", user->token());
    m_config << std::format("device={}\n", user->device());

    m_config.close();
}

std::shared_ptr<User> config_manager::get_user() const
{
    return m_loaded_user;
}

void config_manager::load()
{
    std::string line;
    std::string key;
    std::string value;
    std::string::size_type position;

    m_config.open(m_config_path, std::ios::in);

    if (m_config.is_open())
    {
        if (m_loaded_user == nullptr)
        {
            m_loaded_user = std::make_shared<User>();
        }

        while (std::getline(m_config, line))
        {
            position = line.find("=");

            if (position == std::string::npos)
            {
                continue;
            }

            key = line.substr(0, position);
            value = line.substr(position + 1, std::string::npos);

            if (key == "id")
                m_loaded_user->set_id(std::stoull(value));
            else if (key == "name")
                m_loaded_user->set_name(value);
            else if (key == "email")
                m_loaded_user->set_email(value);
            else if (key == "token")
                m_loaded_user->set_token(value);
            else if (key == "device")
                m_loaded_user->set_device(value);
        }

        m_config.close();
    }
}

void config_manager::make_base()
{
    if (!std::filesystem::exists(m_base_path))
    {
        std::filesystem::create_directories(m_base_path);
    }
}

std::string config_manager::create_device_id()
{
    std::random_device random_device;
    std::mt19937_64 generator(random_device());
    std::uniform_int_distribution<> distribution(0, 15);

    const char* hex{"0123456789abcdef"};
    std::stringstream stream;

    for (int i = 0; i < 36; i++) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            stream << '-';
        } else {
            stream << hex[distribution(generator)];
        }
    }

    return stream.str();
}
