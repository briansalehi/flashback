#pragma once

#include <cstdint>
#include <string>
#include <chrono>

namespace flashback
{
enum class expertise_level
{
    surface,
    depth,
    origin
};

enum class resource_type
{
    book,
    website,
    course,
    video,
    channel,
    mailing_list,
    manual,
    slides,
    user
};

enum class section_pattern
{
    chapter,
    page,
    course,
    video,
    post
};

enum class resource_condition
{
    draft,
    relevant,
    outdated,
    canonical,
    abandoned
};

enum class card_state
{
    draft,
    completed,
    review,
    approved,
    released,
    rejected
};

struct roadmap
{
    std::uint64_t id;
    std::string name;
    [[nodiscard]] constexpr bool operator<(roadmap const& rhs) const noexcept { return name < rhs.name; }
};

struct subject
{
    std::uint64_t id;
    std::string name;
    [[nodiscard]] constexpr bool operator<(subject const& rhs) const noexcept { return name < rhs.name; }
};

struct milestone
{
    std::uint64_t position;
    std::uint64_t id;
    std::string name;
    expertise_level level;
    [[nodiscard]] constexpr bool operator<(milestone const& rhs) const noexcept { return position < rhs.position; }
};

struct resource
{
    std::uint64_t id;
    std::string name;
    std::string presenter;
    std::string provider;
    std::string link;
    resource_type type;
    section_pattern pattern;
    resource_condition condition;
    std::chrono::time_point<std::chrono::steady_clock, std::chrono::seconds> days;

    [[nodiscard]] constexpr bool operator<(resource const& rhs) const noexcept
    {
        return name < rhs.name && presenter < rhs.presenter;
    }
};

struct topic
{
    std::uint64_t position;
    std::string name;
    [[nodiscard]] constexpr bool operator<(topic const& rhs) const noexcept { return position < rhs.position; }
};

struct section
{
    std::uint64_t id;
    std::string name;
    [[nodiscard]] constexpr bool operator<(section const& rhs) const noexcept { return id < rhs.id; }
};

struct card
{
    std::uint64_t id;
    card_state state;
    std::string heading;
    [[nodiscard]] constexpr bool operator<(card const& rhs) const noexcept { return id < rhs.id; }
};

enum class user_state
{
    active,
    inactive,
    suspended,
    banned
};

struct user
{
    std::uint64_t id;
    std::string name;
    std::string email;
    user_state state;
    bool verified;
    std::chrono::time_point<std::chrono::steady_clock, std::chrono::seconds> joined;
};
} // flashback
