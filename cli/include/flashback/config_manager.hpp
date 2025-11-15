#pragma once

#include <fstream>
#include <flashback/basic_config_manager.hpp>

namespace flashback
{
class config_manager: public basic_config_manager
{
public:
    config_manager();
    ~config_manager() override = default;

    [[nodiscard]] std::filesystem::path base_path() const noexcept override;
    [[nodiscard]] std::filesystem::path config_path() const noexcept override;
    void base_path(std::filesystem::path path) override;
    void reload() override;

    [[nodiscard]] std::shared_ptr<User> get_user() const override;

protected:
    void load_user() override;

private:
    std::filesystem::path m_base_path;
    std::filesystem::path m_config_path;
    std::fstream m_config;
    std::shared_ptr<User> m_loaded_user;
};
} // flashback