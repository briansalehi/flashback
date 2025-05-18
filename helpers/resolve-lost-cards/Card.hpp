#pragma once

#include <string>
#include <cstdint>

struct Card
{
    uint64_t subject_id;
    uint32_t topic_position;
    uint64_t card_id;
    uint32_t card_position;
    std::string level;
    std::string subject;
    std::string topic;
    std::string state;
    std::string heading;
};
