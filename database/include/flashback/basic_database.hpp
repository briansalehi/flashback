#pragma once

#include <memory>
#include <utility>
#include <string>
#include <string_view>
#include <types.pb.h>

namespace flashback
{
class basic_database
{
public:
    virtual ~basic_database() = default;
    [[nodiscard]] virtual std::shared_ptr<Roadmaps> get_roadmaps(uint64_t user_id) = 0;
    [[nodiscard]] virtual std::pair<bool, std::string> create_session(uint64_t user_id, std::string_view token, std::string_view device) = 0;
    [[nodiscard]] virtual uint64_t create_user(std::string_view name, std::string_view email, std::string_view hash) = 0;
    virtual void reset_password(uint64_t user_id, std::string_view hash) = 0;
    [[nodiscard]] virtual std::unique_ptr<User> get_user(uint64_t user_id, std::string_view device) = 0;
    virtual void revoke_sessions_except(uint64_t user_id, std::string_view token) = 0;
};
} // flashback
