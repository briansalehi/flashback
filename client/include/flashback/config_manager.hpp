#pragma once

#include <fstream>
#include <flashback/basic_config_manager.hpp>
#include <types.pb.h>

namespace flashback
{
class config_manager final: public basic_config_manager
{
public:
    config_manager();
    ~config_manager() override = default;

    [[nodiscard]] static std::string create_device_id();

    [[nodiscard]] std::filesystem::path base_path() const noexcept override;
    [[nodiscard]] std::filesystem::path config_path() const noexcept override;
    [[nodiscard]] std::unique_ptr<User> get_user() const override;
    [[nodiscard]] bool has_credentials() const noexcept override;
    void base_path(std::filesystem::path path) override;
    void load() override;
    void store(std::shared_ptr<User> user) override;

protected:
    void make_base() override;

private:
    std::filesystem::path m_base_path;
    std::filesystem::path m_config_path;
    std::fstream m_config;
    std::shared_ptr<User> m_loaded_user;
};
} // flashback