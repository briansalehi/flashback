#pragma once

#include <memory>
#include <filesystem>
#include <types.pb.h>

namespace flashback
{
class basic_config_manager
{
public:
    basic_config_manager() = default;
    virtual ~basic_config_manager() = default;

    [[nodiscard]] virtual std::filesystem::path base_path() const noexcept = 0;
    [[nodiscard]] virtual std::filesystem::path config_path() const noexcept = 0;
    [[nodiscard]] virtual bool has_credentials() const noexcept = 0;
    virtual void base_path(std::filesystem::path path) = 0;
    virtual void load() = 0;
    virtual void store(std::shared_ptr<User> user) = 0;

    [[nodiscard]] virtual std::unique_ptr<User> get_user() const = 0;

protected:
    virtual void make_base() = 0;
};
} // flashback
