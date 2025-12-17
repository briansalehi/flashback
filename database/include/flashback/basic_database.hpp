#pragma once

#include <memory>
#include <string_view>
#include <types.pb.h>

namespace flashback
{
class basic_database
{
public:
    virtual ~basic_database() = default;
    virtual bool create_session(uint64_t user_id, std::string_view token, std::string_view device) = 0;
    [[nodiscard]] virtual uint64_t create_user(std::string_view name, std::string_view email, std::string_view hash) = 0;
    virtual void reset_password(uint64_t user_id, std::string_view hash) = 0;
    [[nodiscard]] virtual std::optional<std::shared_ptr<User>> user_exists(std::string_view email) = 0;
    [[nodiscard]] virtual std::unique_ptr<User> get_user(std::string_view email) = 0;
    [[nodiscard]] virtual std::unique_ptr<User> get_user(uint64_t user_id, std::string_view device) = 0;
    virtual void revoke_session(uint64_t user_id, std::string_view token) = 0;
    virtual void revoke_sessions_except(uint64_t user_id, std::string_view token) = 0;
    [[nodiscard]] virtual std::shared_ptr<GetRoadmapsResponse> get_roadmaps(uint64_t user_id) = 0;
};
} // flashback
