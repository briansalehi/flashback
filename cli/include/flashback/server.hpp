#pragma once

#include <set>
#include <map>
#include <memory>
#include <boost/asio.hpp>
#include <flashback/options.hpp>
#include <flashback/types.hpp>

namespace flashback
{
/**
 * Server handles the following requests:
 *
 * Welcome
 *
 * - Get roadmaps
 * - Create a roadmap
 * - Reorder a roadmap
 * - Remove a roadmap
 *
 * Roadmap
 *
 * - Get milestones
 * - Rename the roadmap
 * - Add a milestone
 * - Reorder a milestone
 * - Drop a milestone from the roadmap
 * - Edit the level of a milestone
 *
 * Subject
 *
 * - Get resources
 * - Add the resource to a subject
 * - Create a resource
 * - Reorder a resource
 * - Drop a resource from the subject
 * - Merge two resources
 *
 * Resource
 *
 * - Get sections
 * - Rename the resource
 * - Remove the resource
 * - Edit the provider of resource
 * - Edit the presenter of resource
 * - Edit the link of resource
 * - Change resource type
 * - Change section pattern
 * - Change condition
 * - Create a section
 * - Reorder a section
 * - Remove a section
 * - Merge two sections
 * - Rename a section
 * - Move a section to another resource
 *
 * Subject
 *
 * - Get topics
 * - Add a resource to the subject
 * - Rename the subject
 * - Remove the subject
 * - Create a topic
 * - Reorder a topic
 * - Remove a topic
 * - Merge two topics
 * - Rename a topic
 * - Move a topic to another subject
 *
 * Section
 *
 * - Get study cards
 * - Create a new card
 * - Reorder cards
 * - Remove a card
 * - Merge two cards
 * - Rename the section
 * - Remove the section
 * - Move a card to another resource/section
 *
 * Topic
 *
 * - Get practice cards
 * - Create a new card
 * - Reorder cards
 * - Remove a card
 * - Merge two cards
 * - Rename the topic
 * - Remove the topic
 * - Move a card to another subject/topic
 *
 * Card
 *
 * - Edit heading
 * - Create a block
 * - Edit a block
 * - Remove a block
 * - Edit the extension of a block
 * - Edit the language of a block
 * - Edit the metadata of a block
 * - Reorder blocks
 * - Merge two blocks
 * - Split a block into two
 *
 */
class server final
{
public:
    explicit server(std::shared_ptr<options> options);
    void connect();

    std::set<roadmap> roadmaps(std::uint64_t user) const;
    std::set<milestone> milestones(std::uint64_t roadmap) const;
    std::set<resource> resources(std::uint64_t user, std::uint64_t subject) const;
    std::set<section> sections(std::uint64_t resource) const;
    std::set<topic> topics(std::uint64_t roadmap, std::uint64_t milestones) const;
    std::map<std::uint64_t, card> study_cards(std::uint64_t resource, std::uint64_t section) const;
    std::map<std::uint64_t, card> practice_cards(std::uint64_t roadmap, std::uint64_t milestone,
                                                 std::uint64_t topic) const;

private:
    std::unique_ptr<boost::asio::io_context> m_context;
    std::unique_ptr<boost::asio::ip::tcp::socket> m_remote;
    std::string m_address;
    std::string m_port;
};
} // flashback
