#include <memory>
#include <ranges>
#include <vector>
#include <sstream>
#include <exception>
#include <algorithm>
#include <filesystem>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <pqxx/pqxx>
#include <flashback/database.hpp>
#include <flashback/exception.hpp>

using testing::A;
using testing::An;
using testing::Eq;
using testing::Ne;
using testing::Gt;
using testing::Lt;
using testing::SizeIs;
using testing::IsEmpty;

class test_database: public testing::Test
{
protected:
    void SetUp() override
    {
        m_connection = std::make_unique<pqxx::connection>("postgres://flashback_client@localhost:5432/flashback_test");
        m_database = std::make_unique<flashback::database>("flashback_client", "flashback_test", "localhost", "5432");
        m_user = std::make_unique<flashback::User>();
        m_user->set_name("Flashback Test User");
        m_user->set_email("user@flashback.eu.com");
        m_user->set_hash(R"($argon2id$v=19$m=262144,t=3,p=1$faiJerPBCLb2TEdTbGv8BQ$M0j9j6ojyIjD9yZ4+lBBNR/WAiWpImUcEcUhCL3u9gc)");
        m_user->set_token(R"(iNFzgSaY2W+q42gM9lNVbB13v0odiLy6WnHbInbuvvE)");
        m_user->set_device(R"(aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee)");
        m_user->set_id(create_user());
    }

    void TearDown() override
    {
        remove_users();
        remove_roadmaps();
        remove_subjects();
        remove_cards();
        remove_resources();
        remove_providers();
        remove_presenters();
    }

    [[nodiscard]] uint64_t create_user() const
    {
        uint64_t user_id{};

        EXPECT_NO_THROW(user_id = m_database->create_user(m_user->name(), m_user->email(), m_user->hash()));
        EXPECT_THAT(user_id, Gt(0));

        return user_id;
    }

    void remove_users() const
    {
        pqxx::work remove_user{*m_connection};
        remove_user.exec("delete from users");
        remove_user.commit();
    }

    void remove_roadmaps() const
    {
        pqxx::work removal(*m_connection);
        removal.exec("delete from roadmaps");
        removal.commit();
    }

    void remove_subjects() const
    {
        pqxx::work removal(*m_connection);
        removal.exec("delete from subjects");
        removal.commit();
    }

    void remove_cards() const
    {
        pqxx::work removal(*m_connection);
        removal.exec("delete from cards");
        removal.commit();
    }

    void remove_resources() const
    {
        pqxx::work removal(*m_connection);
        removal.exec("delete from resources");
        removal.commit();
    }

    void remove_providers() const
    {
        pqxx::work removal(*m_connection);
        removal.exec("delete from providers");
        removal.commit();
    }

    void remove_presenters() const
    {
        pqxx::work removal(*m_connection);
        removal.exec("delete from presenters");
        removal.commit();
    }

    void throw_back_progress(uint64_t const user_id, uint64_t const card_id, std::chrono::days const days)
    {
        m_database->throw_back_progress(user_id, card_id, days.count());
    }

    std::unique_ptr<pqxx::connection> m_connection{nullptr};
    std::shared_ptr<flashback::database> m_database{nullptr};
    std::unique_ptr<flashback::User> m_user{nullptr};
};

TEST_F(test_database, CreateDuplicateUser)
{
    uint64_t user_id{};
    EXPECT_NO_THROW(user_id = m_database->create_user(m_user->name(), m_user->email(), m_user->hash()));
    ASSERT_THAT(user_id, Eq(0));
}

TEST_F(test_database, CreateSession)
{
    ASSERT_TRUE(m_database->create_session(m_user->id(), m_user->token(), m_user->device()));
}

TEST_F(test_database, CreateSessionForNonExistingUser)
{
    uint64_t non_existing_user{};
    ASSERT_FALSE(m_database->create_session(non_existing_user, m_user->token(), m_user->device()));
}

TEST_F(test_database, GetUserWithDevice)
{
    std::unique_ptr<flashback::User> user = m_database->get_user(m_user->token(), m_user->device());

    ASSERT_THAT(user, Eq(nullptr));

    std::string unknown_device{R"(33333333-3333-3333-3333-333333333333)"};
    user = m_database->get_user(m_user->token(), m_user->device());

    ASSERT_THAT(user, Eq(nullptr));
    ASSERT_TRUE(m_database->create_session(m_user->id(), m_user->token(), m_user->device()));

    user = m_database->get_user(m_user->token(), m_user->device());

    ASSERT_THAT(user, Ne(nullptr));
    EXPECT_THAT(user->id(), Gt(0));
    EXPECT_THAT(user->name(), Eq(m_user->name()));
    EXPECT_THAT(user->email(), Eq(m_user->email()));
    EXPECT_THAT(user->hash(), Eq(m_user->hash()));
    EXPECT_THAT(user->token(), Eq(m_user->token()));
    EXPECT_THAT(user->device(), Eq(m_user->device()));
    EXPECT_TRUE(user->password().empty());
    EXPECT_FALSE(user->verified());
}

TEST_F(test_database, GetUserWithEmail)
{
    std::string non_existing_email{"non_existing_user@flashback.eu.com"};
    std::unique_ptr<flashback::User> user{m_database->get_user(non_existing_email)};

    ASSERT_THAT(user, Eq(nullptr));

    user = m_database->get_user(m_user->email());

    ASSERT_THAT(user, Ne(nullptr));
    EXPECT_THAT(user->id(), Gt(0));
    EXPECT_THAT(user->name(), Eq(m_user->name()));
    EXPECT_THAT(user->email(), Eq(m_user->email()));
    EXPECT_THAT(user->hash(), Eq(m_user->hash()));
    EXPECT_TRUE(user->token().empty());
    EXPECT_TRUE(user->device().empty());
    EXPECT_TRUE(user->password().empty());
    EXPECT_FALSE(user->verified());
}

TEST_F(test_database, RevokeSessionsExceptSelectedToken)
{
    std::unique_ptr<flashback::User> user{nullptr};
    std::string token1{R"(1111111111+q42gM9lNVbB13v0odiLy6WnHbInbuvvE)"};
    std::string token2{R"(2222222222+q42gM9lNVbB13v0odiLy6WnHbInbuvvE)"};
    std::string device1{R"(11111111-1111-1111-1111-111111111111)"};
    std::string device2{R"(22222222-2222-2222-2222-222222222222)"};
    bool session1_success;
    bool session2_success;

    ASSERT_TRUE(m_database->create_session(m_user->id(), m_user->token(), m_user->device()));
    ASSERT_TRUE(m_database->create_session(m_user->id(), token1, device1));
    ASSERT_TRUE(m_database->create_session(m_user->id(), token2, device2));

    m_database->revoke_sessions_except(m_user->id(), m_user->token());

    user = m_database->get_user(m_user->token(), m_user->device());
    EXPECT_THAT(user, Ne(nullptr));

    user = m_database->get_user(token1, device1);
    ASSERT_THAT(user, Eq(nullptr));

    user = m_database->get_user(token2, device2);
    EXPECT_THAT(user, Eq(nullptr));
}

TEST_F(test_database, RevokeSessionsWithNonExistingToken)
{
    std::string non_existing_token{R"(3333333333+q42gM9lNVbB13v0odiLy6WnHbInbuvvE)"};
    std::unique_ptr<flashback::User> user{nullptr};

    ASSERT_TRUE(m_database->create_session(m_user->id(), m_user->token(), m_user->device()));

    m_database->revoke_sessions_except(m_user->id(), non_existing_token);

    user = m_database->get_user(non_existing_token, m_user->device());
    ASSERT_EQ(user, nullptr) << "Invalid token should not get us any user";

    user = m_database->get_user(m_user->token(), m_user->device());
    ASSERT_NE(user, nullptr) << "Revoking any token should not interfere with other existing tokens";
}

TEST_F(test_database, RevokeSingleSession)
{
    std::unique_ptr<flashback::User> user{nullptr};
    std::string token{R"(1111111111+q42gM9lNVbB13v0odiLy6WnHbInbuvvE)"};
    std::string device{R"(11111111-1111-1111-1111-111111111111)"};

    ASSERT_TRUE(m_database->create_session(m_user->id(), m_user->token(), m_user->device()));
    ASSERT_TRUE(m_database->create_session(m_user->id(), token, device));

    m_database->revoke_session(m_user->id(), token);

    user = m_database->get_user(token, device);
    ASSERT_THAT(user, Eq(nullptr));

    user = m_database->get_user(m_user->token(), m_user->device());
    EXPECT_THAT(user, Ne(nullptr));
}

TEST_F(test_database, ResetPassword)
{
    std::unique_ptr<flashback::User> user{nullptr};
    std::string hash{R"($argon2id$v=19$m=262144,t=3,p=1$faiJerPACLb2TEdTbGv8AQ$M0j9j6ojyIjD9yZ4+lAANR/WAiWpImUcEcUhCL3u9gc)"};

    ASSERT_TRUE(m_database->create_session(m_user->id(), m_user->token(), m_user->device()));

    user = m_database->get_user(m_user->token(), m_user->device());

    ASSERT_THAT(user, Ne(nullptr));
    EXPECT_THAT(user->hash(), Eq(m_user->hash()));

    m_database->reset_password(m_user->id(), hash);

    user = m_database->get_user(m_user->token(), m_user->device());
    ASSERT_THAT(user, Ne(nullptr));
    EXPECT_THAT(user->hash(), Eq(hash));
}

TEST_F(test_database, UserExists)
{
    EXPECT_FALSE(m_database->user_exists("non-existing-user@flashback.eu.com"));
    EXPECT_TRUE(m_database->user_exists(m_user->email()));
    EXPECT_TRUE(m_database->create_session(m_user->id(), m_user->token(), m_user->device()));
    EXPECT_TRUE(m_database->user_exists(m_user->email()));
}

TEST_F(test_database, CreateRoadmap)
{
    flashback::Roadmap primary_roadmap{};
    flashback::Roadmap secondary_roadmap{};
    flashback::Roadmap quoted_roadmap{};
    flashback::Roadmap failed_roadmap{};
    primary_roadmap.set_name("Social Anxiety Expert");
    secondary_roadmap.set_name("Pointless Speech Specialist");
    quoted_roadmap.set_name("O'Reilly Technical Editor");

    EXPECT_NO_THROW(primary_roadmap = m_database->create_roadmap(m_user->id(), primary_roadmap.name()));
    EXPECT_THAT(primary_roadmap.id(), Gt(0));

    EXPECT_NO_THROW(secondary_roadmap = m_database->create_roadmap(m_user->id(), secondary_roadmap.name()));
    EXPECT_THAT(secondary_roadmap.id(), Gt(0));

    EXPECT_THROW(failed_roadmap = m_database->create_roadmap(m_user->id(), primary_roadmap.name()), pqxx::unique_violation);

    EXPECT_NO_THROW(quoted_roadmap = m_database->create_roadmap(m_user->id(), quoted_roadmap.name()));
    EXPECT_THAT(quoted_roadmap.id(), Gt(0));

    EXPECT_THROW(failed_roadmap = m_database->create_roadmap(m_user->id(), ""), flashback::client_exception);
}

TEST_F(test_database, GetRoadmaps)
{
    flashback::Roadmap primary_roadmap{};
    flashback::Roadmap secondary_roadmap{};
    primary_roadmap.set_name("Social Anxiety Expert");
    secondary_roadmap.set_name("Pointless Speech Specialist");
    std::vector<flashback::Roadmap> roadmaps;

    EXPECT_NO_THROW(roadmaps = m_database->get_roadmaps(m_user->id()));
    EXPECT_THAT(roadmaps, testing::IsEmpty()) << "No roadmap was created so far, container should be empty";

    ASSERT_NO_THROW(primary_roadmap = m_database->create_roadmap(m_user->id(), primary_roadmap.name()));
    ASSERT_THAT(primary_roadmap.id(), Gt(0));
    primary_roadmap.set_id(primary_roadmap.id());

    ASSERT_NO_THROW(secondary_roadmap = m_database->create_roadmap(m_user->id(), secondary_roadmap.name()));
    ASSERT_THAT(secondary_roadmap.id(), Gt(0));

    roadmaps = m_database->get_roadmaps(m_user->id());
    EXPECT_THAT(roadmaps, testing::SizeIs(2)) << "Both of the existing roadmaps was assigned to user, so container should contain both";
}

TEST_F(test_database, RenameRoadmap)
{
    flashback::Roadmap roadmap{};
    roadmap.set_name("Over Engineering Expert");
    std::string modified_name{"Task Procrastination Engineer"};
    std::string name_with_quotes{"Underestimated Tasks' Deadline Scheduler"};
    std::vector<flashback::Roadmap> roadmaps{};

    ASSERT_NO_THROW(roadmap = m_database->create_roadmap(m_user->id(), roadmap.name()));
    ASSERT_THAT(roadmap.id(), Gt(0));

    EXPECT_NO_THROW(m_database->rename_roadmap(roadmap.id(), modified_name));
    ASSERT_NO_THROW(roadmaps = m_database->get_roadmaps(m_user->id()));
    EXPECT_THAT(roadmaps.size(), Eq(1));
    ASSERT_NO_THROW(roadmap = roadmaps.at(0));
    EXPECT_THAT(roadmaps.at(0).name(), Eq(modified_name));

    EXPECT_NO_THROW(m_database->rename_roadmap(roadmap.id(), name_with_quotes));
    ASSERT_NO_THROW(roadmaps = m_database->get_roadmaps(m_user->id()));
    EXPECT_THAT(roadmaps.size(), Eq(1));
    ASSERT_NO_THROW(roadmap = roadmaps.at(0));
    EXPECT_THAT(roadmap.name(), Eq(name_with_quotes));

    EXPECT_THROW(m_database->rename_roadmap(roadmap.id(), ""), flashback::client_exception);
}

TEST_F(test_database, RemoveRoadmap)
{
    flashback::Roadmap primary_roadmap{};
    flashback::Roadmap secondary_roadmap{};
    primary_roadmap.set_name("Useless Meetings Creational Engineer");
    secondary_roadmap.set_name("Variable Naming Specialist");
    std::vector<flashback::Roadmap> roadmaps;

    ASSERT_NO_THROW(primary_roadmap = m_database->create_roadmap(m_user->id(), primary_roadmap.name()));
    ASSERT_NO_THROW(secondary_roadmap = m_database->create_roadmap(m_user->id(), secondary_roadmap.name()));
    ASSERT_THAT(primary_roadmap.id(), Gt(0));
    ASSERT_THAT(secondary_roadmap.id(), Gt(0));

    EXPECT_NO_THROW(m_database->remove_roadmap(primary_roadmap.id()));
    ASSERT_THAT(m_database->get_roadmaps(m_user->id()), testing::SizeIs(1)) << "Of two existing roadmaps, one should remain";
    EXPECT_NO_THROW(primary_roadmap = m_database->get_roadmaps(m_user->id()).at(0));
    EXPECT_THAT(primary_roadmap.name(), Eq(secondary_roadmap.name()));

    EXPECT_NO_THROW(m_database->remove_roadmap(primary_roadmap.id())) << "Deleting non-existing roadmap should not throw an exception";
}

TEST_F(test_database, SearchRoadmaps)
{
    std::vector<std::string> const names{
        "Cloud Infrastructure",
        "Quantitative Finance",
        "Database Optimization",
        "Forensic Accounting",
        "Cybersecurity Governance",
        "Data Visualization",
        "Talent Acquisition",
        "Supply Chain Logistics",
        "Intellectual Property Law",
        "Agile Project Management",
        "Full-Stack Development",
        "Backend Development",
        "Digital Marketing",
        "Content Marketing",
        "Financial Analysis",
        "Risk Analysis",
        "Product Management",
        "Project Management",
        "Brand Strategy",
        "Growth Strategy"
    };
    std::map<uint64_t, flashback::Roadmap> search_results;

    for (std::string const& roadmap_name: names)
    {
        flashback::Roadmap roadmap{};
        ASSERT_NO_THROW(roadmap = m_database->create_roadmap(m_user->id(), roadmap_name));
    }

    EXPECT_NO_THROW(search_results = m_database->search_roadmaps("Management"));
    EXPECT_THAT(search_results, testing::SizeIs(testing::Ge(3)));

    EXPECT_NO_THROW(search_results = m_database->search_roadmaps("Prompt Engineering"));
    EXPECT_THAT(search_results, testing::IsEmpty()) << "Should not exist!";
}

TEST_F(test_database, CreateSubject)
{
    flashback::Subject subject;
    std::string subject_name{"Git"};

    EXPECT_NO_THROW(subject = m_database->create_subject(subject_name));
    EXPECT_THAT(subject.name(), Eq(subject_name));
    EXPECT_THAT(subject.id(), Gt(0));

    EXPECT_THROW(subject = m_database->create_subject(subject_name), pqxx::unique_violation) << "Subjects must be unique";

    subject_name = "Linus' Operating System";
    EXPECT_NO_THROW(subject = m_database->create_subject(subject_name)) << "Subjects with quotes in their name should not be a problem";
    EXPECT_THAT(subject.name(), Eq(subject_name));
    EXPECT_THAT(subject.id(), Gt(0));

    subject_name.clear();
    EXPECT_THROW(subject = m_database->create_subject(subject_name), flashback::client_exception) << "Subjects with empty names are not allowed";
}

TEST_F(test_database, SearchSubjects)
{
    std::string const subject_name{"Calculus"};
    std::string const irrelevant_subject{"Linux Administration"};
    std::string const name_pattern{"lus"};
    flashback::Subject subject{};
    std::map<uint64_t, flashback::Subject> matches{};

    ASSERT_NO_THROW(subject = m_database->create_subject(irrelevant_subject));
    ASSERT_EQ(subject.name(), irrelevant_subject) << "Subject sample 1 should be created before performing search";
    ASSERT_THAT(subject.id(), Gt(0));

    ASSERT_NO_THROW(subject = m_database->create_subject(subject_name)) << "Subject sample 2 should be created before performing search";
    ASSERT_THAT(subject.name(), Eq(subject_name));
    ASSERT_THAT(subject.id(), Gt(0));

    EXPECT_NO_THROW(matches = m_database->search_subjects(name_pattern));
    EXPECT_EQ(matches.size(), 1) << "Only one of the two existing objects should be similar";
    EXPECT_NO_THROW(matches.at(1)) << "The first and only match should be in the first position";
    EXPECT_THAT(matches.at(1).id(), Eq(subject.id()));
    EXPECT_THAT(matches.at(1).name(), Eq(subject.name()));

    EXPECT_NO_THROW(matches = m_database->search_subjects(""));
    EXPECT_TRUE(matches.empty()) << "Searching empty name should not have any results";
}

TEST_F(test_database, RenameSubject)
{
    std::string const initial_name{"Container"};
    std::string const modified_name{"Docker"};
    std::string const irrelevant_name{"Linux"};
    std::map<uint64_t, flashback::Subject> matches{};
    constexpr uint64_t expected_position{1};
    flashback::Subject subject{};

    ASSERT_NO_THROW(subject = m_database->create_subject(irrelevant_name));
    ASSERT_THAT(subject.name(), Eq(irrelevant_name));
    ASSERT_THAT(subject.id(), Gt(0));

    ASSERT_NO_THROW(subject = m_database->create_subject(initial_name));
    ASSERT_THAT(subject.name(), Eq(initial_name));
    ASSERT_THAT(subject.id(), Gt(0));

    ASSERT_NO_THROW(matches = m_database->search_subjects(initial_name));
    ASSERT_EQ(matches.size(), 1) << "Irrelevant subject should not be visible in search results, there must be only one matching subject";
    ASSERT_NO_THROW(matches = m_database->search_subjects(modified_name));
    ASSERT_EQ(matches.size(), 0) << "Modified name should not appear before actually modifying";

    EXPECT_NO_THROW(m_database->rename_subject(subject.id(), modified_name));
    EXPECT_NO_THROW(matches = m_database->search_subjects(initial_name));
    EXPECT_EQ(matches.size(), 0) << "Previous name of the subject should not appear";
    EXPECT_NO_THROW(matches = m_database->search_subjects(modified_name));
    ASSERT_EQ(matches.size(), 1) << "New name of the subject should be the only match";
    ASSERT_NO_THROW(matches.at(expected_position));
    EXPECT_THAT(matches.at(expected_position).name(), Eq(modified_name));

    EXPECT_THROW(m_database->rename_subject(subject.id(), ""), flashback::client_exception) << "Empty subject names should be declined";

    EXPECT_THROW(m_database->rename_subject(subject.id(), "Linux"), pqxx::unique_violation) << "Renaming to an existing subject is duplicate and should be declined";
}

TEST_F(test_database, AddMilestone)
{
    std::vector<std::string> const subject_names{"GitHub Actions", "Valgrind", "NeoVim", "OpenSSL"};
    flashback::Roadmap returning_roadmap{};
    flashback::Roadmap expected_roadmap{};
    expected_roadmap.set_name("Open Source Expert");

    ASSERT_NO_THROW(returning_roadmap = m_database->create_roadmap(m_user->id(), expected_roadmap.name()));
    ASSERT_THAT(returning_roadmap.id(), Gt(0));

    for (auto const& name: subject_names)
    {
        flashback::Subject returning_subject;
        flashback::Subject expected_subject;
        flashback::Milestone milestone;
        expected_subject.set_name(name);
        ASSERT_NO_THROW(returning_subject = m_database->create_subject(expected_subject.name()));
        ASSERT_THAT(returning_subject.name(), Eq(expected_subject.name()));
        ASSERT_THAT(returning_subject.id(), Gt(0));
        EXPECT_NO_THROW(milestone = m_database->add_milestone(returning_subject.id(), flashback::expertise_level::surface, returning_roadmap.id()));
        EXPECT_THAT(milestone.id(), Gt(0));
        EXPECT_THAT(milestone.position(), Gt(0));
        EXPECT_THAT(milestone.level(), Eq(flashback::expertise_level::surface));
    }
}

TEST_F(test_database, AddMilestoneWithPosition)
{
    std::vector<std::string> const subject_names{"GitHub Actions", "Valgrind", "NeoVim", "OpenSSL"};
    flashback::Roadmap returning_roadmap{};
    flashback::Roadmap expected_roadmap{};
    expected_roadmap.set_name("Open Source Expert");

    ASSERT_NO_THROW(returning_roadmap = m_database->create_roadmap(m_user->id(), expected_roadmap.name()));
    ASSERT_THAT(returning_roadmap.id(), Gt(0));

    uint64_t position{};

    for (auto const& name: subject_names)
    {
        ++position;
        flashback::Subject returning_subject;
        flashback::Subject expected_subject;
        flashback::Milestone milestone;
        expected_subject.set_name(name);
        ASSERT_NO_THROW(returning_subject = m_database->create_subject(expected_subject.name()));
        ASSERT_THAT(returning_subject.name(), Eq(expected_subject.name()));
        ASSERT_THAT(returning_subject.id(), Gt(0));
        EXPECT_NO_THROW(milestone = m_database->add_milestone(returning_subject.id(), flashback::expertise_level::surface, returning_roadmap.id(), position));
    }
}

TEST_F(test_database, GetMilestones)
{
    std::vector<std::string> const subject_names{"GitHub Actions", "Valgrind", "NeoVim", "OpenSSL"};
    std::vector<flashback::Milestone> milestones{};
    flashback::Roadmap returning_roadmap{};
    flashback::Roadmap expected_roadmap{};
    expected_roadmap.set_name("Open Source Expert");

    ASSERT_NO_THROW(returning_roadmap = m_database->create_roadmap(m_user->id(), expected_roadmap.name()));
    ASSERT_THAT(returning_roadmap.id(), Gt(0));

    for (auto const& name: subject_names)
    {
        flashback::Subject returning_subject;
        flashback::Subject expected_subject;
        flashback::Milestone milestone;
        expected_subject.set_name(name);
        ASSERT_NO_THROW(returning_subject = m_database->create_subject(expected_subject.name()));
        ASSERT_THAT(returning_subject.name(), Eq(expected_subject.name()));
        ASSERT_THAT(returning_subject.id(), Gt(0));
        EXPECT_NO_THROW(milestone = m_database->add_milestone(returning_subject.id(), flashback::expertise_level::surface, returning_roadmap.id()));
        EXPECT_THAT(milestone.id(), Eq(returning_subject.id()));
        EXPECT_THAT(milestone.level(), Eq(flashback::expertise_level::surface));
    }

    EXPECT_NO_THROW(milestones = m_database->get_milestones(returning_roadmap.id()));
    ASSERT_THAT(milestones.size(), Eq(subject_names.size()));

    for (auto const& milestone: milestones)
    {
        EXPECT_THAT(milestone.position(), Gt(0));
        auto const& iter = std::ranges::find_if(subject_names, [&milestone](auto const& name) { return name == milestone.name(); });
        EXPECT_THAT(iter, Ne(subject_names.cend()));
    }
}

TEST_F(test_database, AddRequirement)
{
    flashback::Roadmap roadmap{};
    flashback::Subject dependent_subject{};
    flashback::Subject required_subject{};
    flashback::Milestone dependent_milestone{};
    flashback::Milestone required_milestone{};
    roadmap.set_name("Linux Device Driver Developer");
    dependent_subject.set_name("QEMU");
    required_subject.set_name("Linux System Administration");
    dependent_milestone.set_level(flashback::expertise_level::surface);
    required_milestone.set_level(flashback::expertise_level::depth);

    ASSERT_NO_THROW(roadmap = m_database->create_roadmap(m_user->id(), roadmap.name()));
    ASSERT_THAT(roadmap.id(), Gt(0));
    ASSERT_NO_THROW(dependent_subject = m_database->create_subject(dependent_subject.name()));
    ASSERT_THAT(dependent_subject.id(), Gt(0));
    ASSERT_NO_THROW(required_subject = m_database->create_subject(required_subject.name()));
    ASSERT_THAT(dependent_subject.id(), Gt(0));
    ASSERT_NO_THROW(dependent_milestone = m_database->add_milestone(dependent_subject.id(), dependent_milestone.level(), roadmap.id()));
    ASSERT_THAT(dependent_milestone.id(), Eq(dependent_subject.id()));
    ASSERT_NO_THROW(required_milestone = m_database->add_milestone(required_subject.id(), required_milestone.level(), roadmap.id()));
    ASSERT_THAT(required_milestone.id(), Eq(required_subject.id()));
    EXPECT_NO_THROW(m_database->add_requirement(roadmap.id(), dependent_milestone, required_milestone));
}

TEST_F(test_database, GetRequirements)
{
    flashback::Roadmap roadmap{};
    flashback::Subject dependent_subject{};
    flashback::Subject required_subject{};
    flashback::Milestone dependent_milestone{};
    flashback::Milestone required_milestone{};
    std::vector<flashback::Milestone> requirements{};
    roadmap.set_name("Linux Device Driver Developer");
    dependent_subject.set_name("QEMU");
    required_subject.set_name("Linux System Administration");
    dependent_milestone.set_level(flashback::expertise_level::surface);
    required_milestone.set_level(flashback::expertise_level::depth);

    ASSERT_NO_THROW(roadmap = m_database->create_roadmap(m_user->id(), roadmap.name()));
    ASSERT_THAT(roadmap.id(), Gt(0));
    ASSERT_NO_THROW(dependent_subject = m_database->create_subject(dependent_subject.name()));
    ASSERT_THAT(dependent_subject.id(), Gt(0));
    ASSERT_NO_THROW(required_subject = m_database->create_subject(required_subject.name()));
    ASSERT_THAT(required_subject.id(), Gt(0));
    ASSERT_NO_THROW(dependent_milestone = m_database->add_milestone(dependent_subject.id(), dependent_milestone.level(), roadmap.id()));
    ASSERT_THAT(dependent_milestone.id(), Eq(dependent_subject.id()));
    ASSERT_NO_THROW(required_milestone = m_database->add_milestone(required_subject.id(), required_milestone.level(), roadmap.id()));
    ASSERT_THAT(required_milestone.id(), Eq(required_subject.id()));
    ASSERT_NO_THROW(m_database->add_requirement(roadmap.id(), dependent_milestone, required_milestone));
    EXPECT_NO_THROW(requirements = m_database->get_requirements(roadmap.id(), dependent_milestone.id(), dependent_milestone.level()));
    ASSERT_THAT(requirements.size(), Eq(1));
    ASSERT_NO_THROW(requirements.at(0).id());
    EXPECT_THAT(requirements.at(0).id(), Eq(required_milestone.id()));
}

TEST_F(test_database, CloneRoadmap)
{
    flashback::Roadmap roadmap{};
    flashback::Roadmap cloned_roadmap{};
    std::vector<flashback::Roadmap> roadmaps{};
    std::string token{R"(1111111111+q42gM9lNVbB13v0odiLy6WnHbInbuvvE)"};
    std::string device{R"(ffffffff-gggg-hhhh-iiii-jjjjjjjjjjjj)"};
    uint64_t user_id;

    roadmap.set_name("Theoretical Physicist");

    ASSERT_NO_THROW(user_id = m_database->create_user(m_user->name(), "another@flashback.eu.com", m_user->hash()));
    ASSERT_THAT(user_id, Gt(0));
    ASSERT_TRUE(m_database->create_session(user_id, std::move(token), std::move(device)));
    ASSERT_NO_THROW(roadmap = m_database->create_roadmap(m_user->id(), roadmap.name()));
    ASSERT_THAT(roadmap.id(), Gt(0));
    EXPECT_NO_THROW(cloned_roadmap = m_database->clone_roadmap(user_id, roadmap.id())) << "Roadmap should be cloned for the new user";
    ASSERT_NO_THROW(roadmaps = m_database->get_roadmaps(user_id));
    EXPECT_EQ(roadmaps.size(), 1) << "New user should have one cloned roadmap";
    EXPECT_NO_THROW(roadmaps.at(0).id());
    EXPECT_THAT(roadmaps.at(0).id(), Gt(0));
    EXPECT_NE(roadmaps.at(0).id(), roadmap.id()) << "Cloned roadmap should not have the same ID as the original roadmap";
}

TEST_F(test_database, ReorderMilestone)
{
    flashback::Roadmap roadmap{};
    flashback::Subject subject_pos1{};
    flashback::Subject subject_pos2{};
    flashback::Subject subject_pos3{};
    flashback::Milestone milestone_pos1{};
    flashback::Milestone milestone_pos2{};
    flashback::Milestone milestone_pos3{};
    flashback::expertise_level level{flashback::expertise_level::origin};
    std::vector<flashback::Milestone> milestones{};
    roadmap.set_name("Theoretical Physicist");
    subject_pos1.set_name("Calculus");
    subject_pos2.set_name("Linear Algebra");
    subject_pos3.set_name("Graph Theory");

    ASSERT_NO_THROW(roadmap = m_database->create_roadmap(m_user->id(), roadmap.name()));
    ASSERT_NO_THROW(subject_pos1 = m_database->create_subject(subject_pos1.name()));
    ASSERT_NO_THROW(subject_pos2 = m_database->create_subject(subject_pos2.name()));
    ASSERT_NO_THROW(subject_pos3 = m_database->create_subject(subject_pos3.name()));
    ASSERT_NO_THROW(milestone_pos1 = m_database->add_milestone(subject_pos1.id(), level, roadmap.id()));
    ASSERT_NO_THROW(milestone_pos2 = m_database->add_milestone(subject_pos2.id(), level, roadmap.id()));
    ASSERT_NO_THROW(milestone_pos3 = m_database->add_milestone(subject_pos3.id(), level, roadmap.id()));
    EXPECT_THAT(milestone_pos1.position(), Eq(1));
    EXPECT_THAT(milestone_pos2.position(), Eq(2));
    EXPECT_THAT(milestone_pos3.position(), Eq(3));
    ASSERT_NO_THROW(milestones = m_database->get_milestones(roadmap.id()));
    ASSERT_THAT(milestones, testing::SizeIs(3));
    ASSERT_NO_THROW(milestones.at(0).id());
    ASSERT_NO_THROW(milestones.at(1).id());
    ASSERT_NO_THROW(milestones.at(2).id());
    EXPECT_EQ(milestones.at(0).id(), milestone_pos1.id()) << "Milestone 1 should be in position 1 before reordering";
    EXPECT_EQ(milestones.at(0).position(), 1) << "Vector should have the same order as milestones";
    EXPECT_EQ(milestones.at(1).id(), milestone_pos2.id()) << "Milestone 2 should be in position 2 before reordering";
    EXPECT_EQ(milestones.at(1).position(), 2) << "Vector should have the same order as milestones";
    EXPECT_EQ(milestones.at(2).id(), milestone_pos3.id()) << "Milestone 3 should be in position 3 before reordering";
    EXPECT_EQ(milestones.at(2).position(), 3) << "Vector should have the same order as milestones";
    EXPECT_NO_THROW(m_database->reorder_milestone(roadmap.id(), 1, 3));
    ASSERT_NO_THROW(milestones = m_database->get_milestones(roadmap.id()));
    ASSERT_THAT(milestones, testing::SizeIs(3));
    ASSERT_NO_THROW(milestones.at(0).id());
    ASSERT_NO_THROW(milestones.at(1).id());
    ASSERT_NO_THROW(milestones.at(2).id());
    EXPECT_EQ(milestones.at(0).id(), milestone_pos2.id()) << "Milestone 2 should be in position 1 after reordering";
    EXPECT_EQ(milestones.at(0).position(), 1) << "Vector should have the same order as milestones";
    EXPECT_EQ(milestones.at(1).id(), milestone_pos3.id()) << "Milestone 3 should be in position 2 after reordering";
    EXPECT_EQ(milestones.at(1).position(), 2) << "Vector should have the same order as milestones";
    EXPECT_EQ(milestones.at(2).id(), milestone_pos1.id()) << "Milestone 1 should be in position 3 after reordering";
    EXPECT_EQ(milestones.at(2).position(), 3) << "Vector should have the same order as milestones";
    EXPECT_NO_THROW(m_database->reorder_milestone(roadmap.id(), 3, 1));
    ASSERT_NO_THROW(milestones = m_database->get_milestones(roadmap.id()));
    EXPECT_EQ(milestones.at(0).id(), milestone_pos1.id()) << "Milestone 1 should be in position 1 after reversing the order";
    EXPECT_EQ(milestones.at(0).position(), 1) << "Vector should have the same order as milestones";
    EXPECT_EQ(milestones.at(1).id(), milestone_pos2.id()) << "Milestone 2 should be in position 2 after reversing the order";
    EXPECT_EQ(milestones.at(1).position(), 2) << "Vector should have the same order as milestones";
    EXPECT_EQ(milestones.at(2).id(), milestone_pos3.id()) << "Milestone 3 should be in position 3 after reversing the order";
    EXPECT_EQ(milestones.at(2).position(), 3) << "Vector should have the same order as milestones";
}

TEST_F(test_database, RemoveMilestone)
{
    using testing::SizeIs;

    flashback::Roadmap roadmap{};
    std::vector<flashback::Subject> subjects{};
    std::vector<flashback::Milestone> milestones{};
    roadmap.set_name("Theoretical Physicist");

    ASSERT_NO_THROW(roadmap = m_database->create_roadmap(m_user->id(), roadmap.name()));
    ASSERT_THAT(roadmap.id(), Gt(0));

    for (std::string const& name: {"Calculus", "Linear Algebra", "Graph Theory"})
    {
        flashback::Subject subject{};
        flashback::Milestone milestone{};
        ASSERT_NO_THROW(subject = m_database->create_subject(name));
        ASSERT_THAT(subject.id(), Gt(0));
        ASSERT_NO_THROW(milestone = m_database->add_milestone(subject.id(), flashback::expertise_level::depth, roadmap.id()));
        ASSERT_THAT(milestone.id(), Eq(subject.id()));
    }

    ASSERT_NO_THROW(milestones = m_database->get_milestones(roadmap.id()));
    ASSERT_THAT(milestones, SizeIs(3));
    ASSERT_NO_THROW(milestones.at(2).id());
    EXPECT_NO_THROW(m_database->remove_milestone(roadmap.id(), milestones.at(2).id()));
    ASSERT_NO_THROW(milestones = m_database->get_milestones(roadmap.id()));
    ASSERT_THAT(milestones, SizeIs(2));
    ASSERT_NO_THROW(milestones.at(0).id());
    ASSERT_NO_THROW(milestones.at(1).id());
    EXPECT_THAT(milestones.at(0).id(), Gt(0));
    EXPECT_THAT(milestones.at(1).id(), Gt(0));
}

TEST_F(test_database, ChangeMilestoneLevel)
{
    using testing::SizeIs;

    flashback::Roadmap roadmap{};
    flashback::Subject subject{};
    flashback::Milestone milestone{};
    flashback::Milestone modified_milestone{};
    std::vector<flashback::Milestone> milestones{};
    roadmap.set_name("Theoretical Physicist");

    ASSERT_NO_THROW(roadmap = m_database->create_roadmap(m_user->id(), roadmap.name()));
    ASSERT_THAT(roadmap.id(), Gt(0));

    subject.set_name("Calculus");
    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_THAT(subject.id(), Gt(0));
    ASSERT_NO_THROW(milestone = m_database->add_milestone(subject.id(), flashback::expertise_level::depth, roadmap.id()));
    ASSERT_THAT(milestone.id(), Eq(subject.id()));
    ASSERT_NO_THROW(milestones = m_database->get_milestones(roadmap.id()));
    ASSERT_THAT(milestones, SizeIs(1));
    ASSERT_NO_THROW(modified_milestone = milestones.at(0));
    ASSERT_THAT(modified_milestone.id(), Eq(subject.id()));
    EXPECT_THAT(modified_milestone.level(), Eq(flashback::expertise_level::depth));
    EXPECT_NO_THROW(m_database->change_milestone_level(roadmap.id(), milestone.id(), flashback::expertise_level::surface));
    ASSERT_NO_THROW(milestones = m_database->get_milestones(roadmap.id()));
    ASSERT_THAT(milestones, SizeIs(1));
    ASSERT_NO_THROW(modified_milestone = milestones.at(0));
    ASSERT_THAT(modified_milestone.id(), Eq(subject.id()));
    EXPECT_THAT(modified_milestone.level(), Eq(flashback::expertise_level::surface));
}

TEST_F(test_database, remove_subject)
{
    using testing::SizeIs;

    std::vector<flashback::Subject> subjects{};
    std::map<uint64_t, flashback::Subject> matched_subjects{};

    for (std::string const& name: {"Calculus", "Linear Algebra", "Graph Theory"})
    {
        flashback::Subject sample_subject{};
        EXPECT_NO_THROW(sample_subject = m_database->create_subject(name));
        subjects.push_back(sample_subject);
    }

    flashback::Subject const subject = subjects.at(0);
    ASSERT_THAT(subject.id(), Gt(0));
    ASSERT_FALSE(subject.name().empty());
    EXPECT_NO_THROW(matched_subjects = m_database->search_subjects(subject.name()));
    EXPECT_THAT(matched_subjects, SizeIs(1)) << "Subject should appear in the search result before being removed";
    EXPECT_NO_THROW(m_database->remove_subject(subject.id()));
    EXPECT_NO_THROW(matched_subjects = m_database->search_subjects(subject.name()));
    EXPECT_THAT(matched_subjects, SizeIs(0)) << "There should be no search results for the subject that was removed earlier";
}

TEST_F(test_database, merge_subjects)
{
    using testing::SizeIs;

    flashback::Subject source_subject{};
    flashback::Subject target_subject{};
    std::map<uint64_t, flashback::Subject> matched_subjects{};
    source_subject.set_name("Basic Mathematics");
    target_subject.set_name("Calculus");

    ASSERT_NO_THROW(source_subject = m_database->create_subject(source_subject.name()));
    ASSERT_NO_THROW(target_subject = m_database->create_subject(target_subject.name()));
    EXPECT_NO_THROW(matched_subjects = m_database->search_subjects(source_subject.name()));
    ASSERT_THAT(matched_subjects, SizeIs(1));
    ASSERT_NO_THROW(matched_subjects.at(1));
    EXPECT_THAT(matched_subjects.at(1).id(), source_subject.id());
    EXPECT_THAT(matched_subjects.at(1).name(), source_subject.name());
    ASSERT_NO_THROW(matched_subjects = m_database->search_subjects(target_subject.name()));
    ASSERT_THAT(matched_subjects, SizeIs(1));
    ASSERT_NO_THROW(matched_subjects.at(1));
    EXPECT_THAT(matched_subjects.at(1).id(), target_subject.id());
    EXPECT_THAT(matched_subjects.at(1).name(), target_subject.name());
    EXPECT_NO_THROW(m_database->merge_subjects(source_subject.id(), target_subject.id()));
}

TEST_F(test_database, create_resource)
{
    uint64_t id{};
    constexpr auto name{"Introduction to Algorithms"};
    constexpr auto type{flashback::Resource::book};
    constexpr auto pattern{flashback::Resource::chapter};
    constexpr auto link{R"(https://example.com)"};
    auto const now{std::chrono::system_clock::now().time_since_epoch()};
    auto const later{std::chrono::system_clock::time_point(std::chrono::system_clock::now() + std::chrono::years{3}).time_since_epoch()};
    auto const production{std::chrono::duration_cast<std::chrono::seconds>(now).count()};
    auto const expiration{std::chrono::duration_cast<std::chrono::seconds>(later).count()};
    flashback::Resource resource{};
    resource.set_name(name);
    resource.set_type(type);
    resource.set_pattern(pattern);
    resource.set_link(link);
    resource.set_production(production);
    resource.set_expiration(expiration);

    EXPECT_NO_THROW(resource = m_database->create_resource(resource));
    EXPECT_THAT(resource.id(), Gt(0));
    EXPECT_THAT(resource.name(), Eq(name));
    EXPECT_THAT(resource.type(), Eq(type));
    EXPECT_THAT(resource.pattern(), Eq(pattern));
    EXPECT_THAT(resource.link(), Eq(link));
    EXPECT_THAT(resource.production(), Eq(production));
    EXPECT_THAT(resource.expiration(), Eq(expiration));

    resource.clear_id();
    resource.clear_name();
    EXPECT_NO_THROW(resource = m_database->create_resource(resource));
    EXPECT_EQ(resource.id(), 0) << "Resource with empty name should be created";
    EXPECT_TRUE(resource.name().empty());
    EXPECT_THAT(resource.type(), Eq(type));
    EXPECT_THAT(resource.pattern(), Eq(pattern));
    EXPECT_THAT(resource.link(), Eq(link));
    EXPECT_THAT(resource.production(), Eq(production));
    EXPECT_THAT(resource.expiration(), Eq(expiration));
}

TEST_F(test_database, add_resource_to_subject)
{
    flashback::Subject subject{};
    flashback::Resource resource{};
    auto const now{std::chrono::system_clock::now().time_since_epoch()};
    auto const later{std::chrono::system_clock::time_point(std::chrono::system_clock::now() + std::chrono::years{3}).time_since_epoch()};
    auto const production{std::chrono::duration_cast<std::chrono::seconds>(now).count()};
    auto const expiration{std::chrono::duration_cast<std::chrono::seconds>(later).count()};
    resource.set_name("Introduction to Algorithms");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.set_link(R"(https://example.com)");
    resource.set_production(production);
    resource.set_expiration(expiration);
    subject.set_name("Algorithms");

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_THAT(subject.id(), Gt(0));
    EXPECT_NO_THROW(m_database->add_resource_to_subject(resource.id(), subject.id()));
}

TEST_F(test_database, get_resources)
{
    using testing::SizeIs;

    flashback::Subject subject{};
    flashback::Resource resource{};
    std::vector<flashback::Resource> resources{};
    auto const now{std::chrono::system_clock::now().time_since_epoch()};
    auto const later{std::chrono::system_clock::time_point(std::chrono::system_clock::now() + std::chrono::years{3}).time_since_epoch()};
    auto const production{std::chrono::duration_cast<std::chrono::seconds>(now).count()};
    auto const expiration{std::chrono::duration_cast<std::chrono::seconds>(later).count()};
    resource.set_name("Introduction to Algorithms");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.set_link(R"(https://example.com)");
    resource.set_production(production);
    resource.set_expiration(expiration);
    flashback::Resource secondary_resource{};
    secondary_resource.set_name("You Don't Know JS");
    secondary_resource.set_type(flashback::Resource::book);
    secondary_resource.set_pattern(flashback::Resource::chapter);
    secondary_resource.set_link(R"(https://example.com)");
    secondary_resource.set_production(production);
    secondary_resource.set_expiration(expiration);
    subject.set_name("Algorithms");

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(secondary_resource = m_database->create_resource(secondary_resource));
    ASSERT_THAT(secondary_resource.id(), Gt(0));
    ASSERT_THAT(resource.id(), Ne(secondary_resource.id()));
    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_THAT(subject.id(), Gt(0));
    EXPECT_NO_THROW(resources = m_database->get_resources(m_user->id(), subject.id() + 1));
    EXPECT_THAT(resources, SizeIs(0)) << "Invalid subject should not have resources";
    ASSERT_NO_THROW(m_database->add_resource_to_subject(resource.id(), subject.id()));
    ASSERT_NO_THROW(m_database->add_resource_to_subject(secondary_resource.id(), subject.id()));
    EXPECT_NO_THROW(resources = m_database->get_resources(m_user->id(), subject.id()));
    EXPECT_THAT(resources, SizeIs(2));
    ASSERT_NO_THROW(resources.at(0).id());
    EXPECT_THAT(resources.at(0).id(), Eq(resource.id()));
    EXPECT_THAT(resources.at(0).name(), Eq(resource.name()));
    EXPECT_THAT(resources.at(0).type(), Eq(resource.type()));
    EXPECT_THAT(resources.at(0).pattern(), Eq(resource.pattern()));
    EXPECT_THAT(resources.at(0).link(), Eq(resource.link()));
    EXPECT_THAT(resources.at(0).production(), Eq(resource.production()));
    EXPECT_THAT(resources.at(0).expiration(), Eq(resource.expiration()));
}

TEST_F(test_database, drop_resource_from_subject)
{
    using testing::SizeIs;

    flashback::Subject subject{};
    std::vector<flashback::Resource> resources{};
    auto const now{std::chrono::system_clock::now().time_since_epoch()};
    auto const later{std::chrono::system_clock::time_point(std::chrono::system_clock::now() + std::chrono::years{3}).time_since_epoch()};
    auto const production{std::chrono::duration_cast<std::chrono::seconds>(now).count()};
    auto const expiration{std::chrono::duration_cast<std::chrono::seconds>(later).count()};
    flashback::Resource resource{};
    resource.set_name("Introduction to Algorithms");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.set_link(R"(https://example.com)");
    resource.set_production(production);
    resource.set_expiration(expiration);
    flashback::Resource secondary_resource{};
    secondary_resource.set_name("You Don't Know JS");
    secondary_resource.set_type(flashback::Resource::book);
    secondary_resource.set_pattern(flashback::Resource::chapter);
    secondary_resource.set_link(R"(https://example.com)");
    secondary_resource.set_production(production);
    secondary_resource.set_expiration(expiration);
    subject.set_name("JavaScript");

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(secondary_resource = m_database->create_resource(secondary_resource));
    ASSERT_THAT(secondary_resource.id(), Gt(0));
    ASSERT_THAT(resource.id(), Ne(secondary_resource.id()));
    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_THAT(subject.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_resource_to_subject(resource.id(), subject.id()));
    EXPECT_NO_THROW(resources = m_database->get_resources(m_user->id(), subject.id()));
    EXPECT_THAT(resources, SizeIs(1));
    resources.clear();
    ASSERT_NO_THROW(m_database->add_resource_to_subject(secondary_resource.id(), subject.id()));
    EXPECT_NO_THROW(resources = m_database->get_resources(m_user->id(), subject.id()));
    EXPECT_THAT(resources, SizeIs(2));
    EXPECT_NO_THROW(m_database->drop_resource_from_subject(resource.id(), subject.id()));
    EXPECT_NO_THROW(resources = m_database->get_resources(m_user->id(), subject.id()));
    EXPECT_THAT(resources, SizeIs(1));
    EXPECT_THAT(resources.at(0).id(), secondary_resource.id());
}

TEST_F(test_database, search_resources)
{
    using testing::SizeIs;

    constexpr auto search_pattern{"cmake"};
    constexpr auto irrelevant_name{"Irrelevant"};
    auto const now{std::chrono::system_clock::now().time_since_epoch()};
    auto const later{std::chrono::system_clock::time_point(std::chrono::system_clock::now() + std::chrono::years{3}).time_since_epoch()};
    auto const production{std::chrono::duration_cast<std::chrono::seconds>(now).count()};
    auto const expiration{std::chrono::duration_cast<std::chrono::seconds>(later).count()};
    std::vector<flashback::Resource> resources{};
    std::map<uint64_t, flashback::Resource> matched_resources{};
    flashback::Subject subject{};
    subject.set_name("CMake");

    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_THAT(subject.id(), Gt(0));

    for (std::string const& name: {"Modern CMake", "Old CMake", "CMake Alternatives", "CMake for C++", "GNU Make", "Make Manual", "Irrelevant"})
    {
        flashback::Resource resource{};
        resource.set_name(name);
        resource.set_type(flashback::Resource::book);
        resource.set_pattern(flashback::Resource::chapter);
        resource.set_link(R"(https://example.com)");
        resource.set_production(production);
        resource.set_expiration(expiration);
        ASSERT_NO_THROW(resource = m_database->create_resource(resource));
        ASSERT_THAT(resource.id(), Gt(0));
        ASSERT_NO_THROW(m_database->add_resource_to_subject(resource.id(), subject.id()));
        resources.push_back(std::move(resource));
    }

    EXPECT_NO_THROW(resources = m_database->get_resources(m_user->id(), subject.id()));
    EXPECT_THAT(resources, SizeIs(7));
    EXPECT_NO_THROW(matched_resources = m_database->search_resources(search_pattern));
    EXPECT_THAT(matched_resources, SizeIs(6));
    EXPECT_NO_THROW(matched_resources = m_database->search_resources(irrelevant_name));
    EXPECT_THAT(matched_resources, SizeIs(1));
}

TEST_F(test_database, edit_resource_link)
{
    using testing::SizeIs;

    constexpr auto modified_link{R"(https:://modified.com)"};
    flashback::Subject subject{};
    std::vector<flashback::Resource> resources{};
    auto const now{std::chrono::system_clock::now().time_since_epoch()};
    auto const later{std::chrono::system_clock::time_point(std::chrono::system_clock::now() + std::chrono::years{3}).time_since_epoch()};
    auto const production{std::chrono::duration_cast<std::chrono::seconds>(now).count()};
    auto const expiration{std::chrono::duration_cast<std::chrono::seconds>(later).count()};
    flashback::Resource resource{};
    resource.set_name("Introduction to Algorithms");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.set_link(R"(https://example.com)");
    resource.set_production(production);
    resource.set_expiration(expiration);
    subject.set_name("JavaScript");

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_THAT(subject.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_resource_to_subject(resource.id(), subject.id()));
    ASSERT_NO_THROW(resources = m_database->get_resources(m_user->id(), subject.id()));
    ASSERT_THAT(resources, SizeIs(1));
    ASSERT_NO_THROW(resources.at(0).id());
    ASSERT_THAT(resources.at(0).id(), Eq(resource.id()));
    EXPECT_THAT(resources.at(0).link(), Eq(resource.link()));
    EXPECT_THAT(resources.at(0).link(), Ne(modified_link));
    EXPECT_NO_THROW(m_database->edit_resource_link(resource.id(), modified_link));
    ASSERT_NO_THROW(resources = m_database->get_resources(m_user->id(), subject.id()));
    ASSERT_THAT(resources, SizeIs(1));
    ASSERT_NO_THROW(resources.at(0).id());
    ASSERT_THAT(resources.at(0).id(), Eq(resource.id()));
    EXPECT_THAT(resources.at(0).link(), Ne(resource.link()));
    EXPECT_THAT(resources.at(0).link(), Eq(modified_link));
}

TEST_F(test_database, change_resource_type)
{
    using testing::SizeIs;

    flashback::Resource::resource_type modified_type{flashback::Resource::channel};
    flashback::Subject subject{};
    std::vector<flashback::Resource> resources{};
    auto const now{std::chrono::system_clock::now().time_since_epoch()};
    auto const later{std::chrono::system_clock::time_point(std::chrono::system_clock::now() + std::chrono::years{3}).time_since_epoch()};
    auto const production{std::chrono::duration_cast<std::chrono::seconds>(now).count()};
    auto const expiration{std::chrono::duration_cast<std::chrono::seconds>(later).count()};
    flashback::Resource resource{};
    resource.set_name("Introduction to Algorithms");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.set_link(R"(https://example.com)");
    resource.set_production(production);
    resource.set_expiration(expiration);
    subject.set_name("JavaScript");

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_THAT(subject.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_resource_to_subject(resource.id(), subject.id()));
    ASSERT_NO_THROW(resources = m_database->get_resources(m_user->id(), subject.id()));
    ASSERT_THAT(resources, SizeIs(1));
    ASSERT_NO_THROW(resources.at(0).id());
    ASSERT_THAT(resources.at(0).id(), Eq(resource.id()));
    EXPECT_THAT(resources.at(0).type(), Eq(resource.type()));
    EXPECT_THAT(resources.at(0).type(), Ne(modified_type));
    EXPECT_NO_THROW(m_database->change_resource_type(resource.id(), modified_type));
    ASSERT_NO_THROW(resources = m_database->get_resources(m_user->id(), subject.id()));
    ASSERT_THAT(resources, SizeIs(1));
    ASSERT_NO_THROW(resources.at(0).id());
    ASSERT_THAT(resources.at(0).id(), Eq(resource.id()));
    EXPECT_THAT(resources.at(0).type(), Ne(resource.type()));
    EXPECT_THAT(resources.at(0).type(), Eq(modified_type));
}

TEST_F(test_database, change_section_pattern)
{
    using testing::SizeIs;

    flashback::Resource::section_pattern modified_pattern{flashback::Resource::video};
    flashback::Subject subject{};
    std::vector<flashback::Resource> resources{};
    auto const now{std::chrono::system_clock::now().time_since_epoch()};
    auto const later{std::chrono::system_clock::time_point(std::chrono::system_clock::now() + std::chrono::years{3}).time_since_epoch()};
    auto const production{std::chrono::duration_cast<std::chrono::seconds>(now).count()};
    auto const expiration{std::chrono::duration_cast<std::chrono::seconds>(later).count()};
    flashback::Resource resource{};
    resource.set_name("Introduction to Algorithms");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.set_link(R"(https://example.com)");
    resource.set_production(production);
    resource.set_expiration(expiration);
    subject.set_name("JavaScript");

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_THAT(subject.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_resource_to_subject(resource.id(), subject.id()));
    ASSERT_NO_THROW(resources = m_database->get_resources(m_user->id(), subject.id()));
    ASSERT_THAT(resources, SizeIs(1));
    ASSERT_NO_THROW(resources.at(0).id());
    ASSERT_THAT(resources.at(0).id(), Eq(resource.id()));
    EXPECT_THAT(resources.at(0).pattern(), Eq(resource.pattern()));
    EXPECT_THAT(resources.at(0).pattern(), Ne(modified_pattern));
    EXPECT_NO_THROW(m_database->change_section_pattern(resource.id(), modified_pattern));
    ASSERT_NO_THROW(resources = m_database->get_resources(m_user->id(), subject.id()));
    ASSERT_THAT(resources, SizeIs(1));
    ASSERT_NO_THROW(resources.at(0).id());
    ASSERT_THAT(resources.at(0).id(), Eq(resource.id()));
    EXPECT_THAT(resources.at(0).pattern(), Ne(resource.pattern()));
    EXPECT_THAT(resources.at(0).pattern(), Eq(modified_pattern));
}

TEST_F(test_database, edit_resource_production)
{
    using testing::SizeIs;

    flashback::Subject subject{};
    std::vector<flashback::Resource> resources{};
    auto const now{std::chrono::system_clock::now().time_since_epoch()};
    auto const later{std::chrono::system_clock::time_point(std::chrono::system_clock::now() + std::chrono::years{3}).time_since_epoch()};
    auto const production{std::chrono::duration_cast<std::chrono::seconds>(now).count()};
    auto const modified_production{std::chrono::duration_cast<std::chrono::seconds>(now + std::chrono::days{2}).count()};
    auto const expiration{std::chrono::duration_cast<std::chrono::seconds>(later).count()};
    flashback::Resource resource{};
    resource.set_name("Introduction to Algorithms");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.set_link(R"(https://example.com)");
    resource.set_production(production);
    resource.set_expiration(expiration);
    subject.set_name("JavaScript");

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_THAT(subject.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_resource_to_subject(resource.id(), subject.id()));
    ASSERT_NO_THROW(resources = m_database->get_resources(m_user->id(), subject.id()));
    ASSERT_THAT(resources, SizeIs(1));
    ASSERT_NO_THROW(resources.at(0).id());
    ASSERT_THAT(resources.at(0).id(), Eq(resource.id()));
    EXPECT_THAT(resources.at(0).production(), Eq(resource.production()));
    EXPECT_THAT(resources.at(0).production(), Ne(modified_production));
    EXPECT_NO_THROW(m_database->edit_resource_production(resource.id(), modified_production));
    ASSERT_NO_THROW(resources = m_database->get_resources(m_user->id(), subject.id()));
    ASSERT_THAT(resources, SizeIs(1));
    ASSERT_NO_THROW(resources.at(0).id());
    ASSERT_THAT(resources.at(0).id(), Eq(resource.id()));
    EXPECT_THAT(resources.at(0).production(), Ne(resource.production()));
    EXPECT_THAT(resources.at(0).production(), Eq(modified_production));
}

TEST_F(test_database, edit_resource_expiration)
{
    using testing::SizeIs;

    flashback::Subject subject{};
    std::vector<flashback::Resource> resources{};
    auto const now{std::chrono::system_clock::now().time_since_epoch()};
    auto const later{std::chrono::system_clock::time_point(std::chrono::system_clock::now() + std::chrono::years{3}).time_since_epoch()};
    auto const production{std::chrono::duration_cast<std::chrono::seconds>(now).count()};
    auto const expiration{std::chrono::duration_cast<std::chrono::seconds>(later).count()};
    auto const modified_expiration{std::chrono::duration_cast<std::chrono::seconds>(later + std::chrono::days{2}).count()};
    flashback::Resource resource{};
    resource.set_name("Introduction to Algorithms");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.set_link(R"(https://example.com)");
    resource.set_production(production);
    resource.set_expiration(expiration);
    subject.set_name("JavaScript");

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_THAT(subject.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_resource_to_subject(resource.id(), subject.id()));
    ASSERT_NO_THROW(resources = m_database->get_resources(m_user->id(), subject.id()));
    ASSERT_THAT(resources, SizeIs(1));
    ASSERT_NO_THROW(resources.at(0).id());
    ASSERT_THAT(resources.at(0).id(), Eq(resource.id()));
    EXPECT_THAT(resources.at(0).expiration(), Eq(resource.expiration()));
    EXPECT_THAT(resources.at(0).expiration(), Ne(modified_expiration));
    EXPECT_NO_THROW(m_database->edit_resource_expiration(resource.id(), modified_expiration));
    ASSERT_NO_THROW(resources = m_database->get_resources(m_user->id(), subject.id()));
    ASSERT_THAT(resources, SizeIs(1));
    ASSERT_NO_THROW(resources.at(0).id());
    ASSERT_THAT(resources.at(0).id(), Eq(resource.id()));
    EXPECT_THAT(resources.at(0).expiration(), Ne(resource.expiration()));
    EXPECT_THAT(resources.at(0).expiration(), Eq(modified_expiration));
}

TEST_F(test_database, rename_resource)
{
    using testing::SizeIs;

    flashback::Subject subject{};
    std::vector<flashback::Resource> resources{};
    auto const now{std::chrono::system_clock::now().time_since_epoch()};
    auto const later{std::chrono::system_clock::time_point(std::chrono::system_clock::now() + std::chrono::years{3}).time_since_epoch()};
    auto const production{std::chrono::duration_cast<std::chrono::seconds>(now).count()};
    auto const expiration{std::chrono::duration_cast<std::chrono::seconds>(later).count()};
    auto const modified_name{"The End of Algorithms"};
    flashback::Resource resource{};
    resource.set_name("Introduction to Algorithms");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.set_link(R"(https://example.com)");
    resource.set_production(production);
    resource.set_expiration(expiration);
    subject.set_name("JavaScript");

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_THAT(subject.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_resource_to_subject(resource.id(), subject.id()));
    ASSERT_NO_THROW(resources = m_database->get_resources(m_user->id(), subject.id()));
    ASSERT_THAT(resources, SizeIs(1));
    ASSERT_NO_THROW(resources.at(0).id());
    ASSERT_THAT(resources.at(0).id(), Eq(resource.id()));
    EXPECT_THAT(resources.at(0).name(), Eq(resource.name()));
    EXPECT_THAT(resources.at(0).name(), Ne(modified_name));
    EXPECT_NO_THROW(m_database->rename_resource(resource.id(), modified_name));
    ASSERT_NO_THROW(resources = m_database->get_resources(m_user->id(), subject.id()));
    ASSERT_THAT(resources, SizeIs(1));
    ASSERT_NO_THROW(resources.at(0).id());
    ASSERT_THAT(resources.at(0).id(), Eq(resource.id()));
    EXPECT_THAT(resources.at(0).name(), Ne(resource.name()));
    EXPECT_THAT(resources.at(0).name(), Eq(modified_name));
}

TEST_F(test_database, remove_resource)
{
    using testing::SizeIs;

    flashback::Subject subject{};
    std::vector<flashback::Resource> resources{};
    auto const now{std::chrono::system_clock::now().time_since_epoch()};
    auto const later{std::chrono::system_clock::time_point(std::chrono::system_clock::now() + std::chrono::years{3}).time_since_epoch()};
    auto const production{std::chrono::duration_cast<std::chrono::seconds>(now).count()};
    auto const expiration{std::chrono::duration_cast<std::chrono::seconds>(later).count()};
    flashback::Resource resource{};
    resource.set_name("Introduction to Algorithms");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.set_link(R"(https://example.com)");
    resource.set_production(production);
    resource.set_expiration(expiration);
    flashback::Resource secondary_resource{};
    secondary_resource.set_name("You Don't Know JS");
    secondary_resource.set_type(flashback::Resource::book);
    secondary_resource.set_pattern(flashback::Resource::chapter);
    secondary_resource.set_link(R"(https://example.com)");
    secondary_resource.set_production(production);
    secondary_resource.set_expiration(expiration);
    subject.set_name("JavaScript");

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(secondary_resource = m_database->create_resource(secondary_resource));
    ASSERT_THAT(secondary_resource.id(), Gt(0));
    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_THAT(subject.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_resource_to_subject(resource.id(), subject.id()));
    ASSERT_NO_THROW(m_database->add_resource_to_subject(secondary_resource.id(), subject.id()));
    ASSERT_NO_THROW(resources = m_database->get_resources(m_user->id(), subject.id()));
    ASSERT_THAT(resources, SizeIs(2));
    EXPECT_NO_THROW(m_database->remove_resource(resource.id()));
    ASSERT_NO_THROW(resources = m_database->get_resources(m_user->id(), subject.id()));
    ASSERT_THAT(resources, SizeIs(1));
}

TEST_F(test_database, merge_resources)
{
    using testing::SizeIs;

    flashback::Subject subject{};
    std::vector<flashback::Resource> resources{};
    auto const now{std::chrono::system_clock::now().time_since_epoch()};
    auto const later{std::chrono::system_clock::time_point(std::chrono::system_clock::now() + std::chrono::years{3}).time_since_epoch()};
    auto const production{std::chrono::duration_cast<std::chrono::seconds>(now).count()};
    auto const expiration{std::chrono::duration_cast<std::chrono::seconds>(later).count()};
    flashback::Resource resource{};
    resource.set_name("Introduction to Algorithms");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.set_link(R"(https://example.com)");
    resource.set_production(production);
    resource.set_expiration(expiration);
    flashback::Resource secondary_resource{};
    secondary_resource.set_name("You Don't Know JS");
    secondary_resource.set_type(flashback::Resource::book);
    secondary_resource.set_pattern(flashback::Resource::chapter);
    secondary_resource.set_link(R"(https://example.com)");
    secondary_resource.set_production(production);
    secondary_resource.set_expiration(expiration);
    subject.set_name("JavaScript");

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(secondary_resource = m_database->create_resource(secondary_resource));
    ASSERT_THAT(secondary_resource.id(), Gt(0));
    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_THAT(subject.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_resource_to_subject(resource.id(), subject.id()));
    ASSERT_NO_THROW(m_database->add_resource_to_subject(secondary_resource.id(), subject.id()));
    ASSERT_NO_THROW(resources = m_database->get_resources(m_user->id(), subject.id()));
    ASSERT_THAT(resources, SizeIs(2));
    EXPECT_NO_THROW(m_database->merge_resources(resource.id(), secondary_resource.id()));
    ASSERT_NO_THROW(resources = m_database->get_resources(m_user->id(), subject.id()));
    ASSERT_THAT(resources, SizeIs(1));
}

TEST_F(test_database, create_provider)
{
    std::string const name{"Brian Salehi"};
    flashback::Provider provider{};
    provider.set_name(name);
    EXPECT_NO_THROW(provider = m_database->create_provider(provider.name()));
    EXPECT_THAT(provider.id(), Gt(0));
    EXPECT_THAT(provider.name(), Eq(name));
}

TEST_F(test_database, add_provider)
{
    using testing::SizeIs;

    std::vector<flashback::Resource> resources{};
    auto const now{std::chrono::system_clock::now().time_since_epoch()};
    auto const later{std::chrono::system_clock::time_point(std::chrono::system_clock::now() + std::chrono::years{3}).time_since_epoch()};
    auto const production{std::chrono::duration_cast<std::chrono::seconds>(now).count()};
    auto const expiration{std::chrono::duration_cast<std::chrono::seconds>(later).count()};
    flashback::Resource resource{};
    resource.set_name("Introduction to Algorithms");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.set_link(R"(https://example.com)");
    resource.set_production(production);
    resource.set_expiration(expiration);
    flashback::Provider provider{};
    provider.set_name("Brian Salehi");

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(provider = m_database->create_provider(provider.name()));
    ASSERT_THAT(provider.id(), Gt(0));
    EXPECT_NO_THROW(m_database->add_provider(resource.id(), provider.id()));
}

TEST_F(test_database, drop_provider)
{
    using testing::SizeIs;

    std::vector<flashback::Resource> resources{};
    auto const now{std::chrono::system_clock::now().time_since_epoch()};
    auto const later{std::chrono::system_clock::time_point(std::chrono::system_clock::now() + std::chrono::years{3}).time_since_epoch()};
    auto const production{std::chrono::duration_cast<std::chrono::seconds>(now).count()};
    auto const expiration{std::chrono::duration_cast<std::chrono::seconds>(later).count()};
    flashback::Resource resource{};
    resource.set_name("Introduction to Algorithms");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.set_link(R"(https://example.com)");
    resource.set_production(production);
    resource.set_expiration(expiration);
    flashback::Provider provider{};
    provider.set_name("Brian Salehi");

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(provider = m_database->create_provider(provider.name()));
    ASSERT_THAT(provider.id(), Gt(0));
    EXPECT_NO_THROW(m_database->drop_provider(resource.id(), provider.id()));
}

TEST_F(test_database, create_presenter)
{
    std::string const name{"Brian Salehi"};
    flashback::Presenter presenter{};
    presenter.set_name(name);
    EXPECT_NO_THROW(presenter = m_database->create_presenter(presenter.name()));
    EXPECT_THAT(presenter.id(), Gt(0));
    EXPECT_THAT(presenter.name(), Eq(name));
}

TEST_F(test_database, add_presenter)
{
    using testing::SizeIs;

    std::vector<flashback::Resource> resources{};
    auto const now{std::chrono::system_clock::now().time_since_epoch()};
    auto const later{std::chrono::system_clock::time_point(std::chrono::system_clock::now() + std::chrono::years{3}).time_since_epoch()};
    auto const production{std::chrono::duration_cast<std::chrono::seconds>(now).count()};
    auto const expiration{std::chrono::duration_cast<std::chrono::seconds>(later).count()};
    flashback::Resource resource{};
    resource.set_name("Introduction to Algorithms");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.set_link(R"(https://example.com)");
    resource.set_production(production);
    resource.set_expiration(expiration);
    flashback::Presenter presenter{};
    presenter.set_name("Brian Salehi");

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(presenter = m_database->create_presenter(presenter.name()));
    ASSERT_THAT(presenter.id(), Gt(0));
    EXPECT_NO_THROW(m_database->add_presenter(resource.id(), presenter.id()));
}

TEST_F(test_database, drop_presenter)
{
    using testing::SizeIs;

    std::vector<flashback::Resource> resources{};
    auto const now{std::chrono::system_clock::now().time_since_epoch()};
    auto const later{std::chrono::system_clock::time_point(std::chrono::system_clock::now() + std::chrono::years{3}).time_since_epoch()};
    auto const production{std::chrono::duration_cast<std::chrono::seconds>(now).count()};
    auto const expiration{std::chrono::duration_cast<std::chrono::seconds>(later).count()};
    flashback::Resource resource{};
    resource.set_name("Introduction to Algorithms");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.set_link(R"(https://example.com)");
    resource.set_production(production);
    resource.set_expiration(expiration);
    flashback::Presenter presenter{};
    presenter.set_name("Brian Salehi");

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(presenter = m_database->create_presenter(presenter.name()));
    ASSERT_THAT(presenter.id(), Gt(0));
    EXPECT_NO_THROW(m_database->drop_presenter(resource.id(), presenter.id()));
}

TEST_F(test_database, search_providers)
{
    using testing::SizeIs;
    using testing::IsEmpty;

    std::vector<flashback::Provider> existing_providers{};
    std::map<uint64_t, flashback::Provider> matched_providers{};

    for (auto const& name: {"John Doe", "Jane Doe", "Brian Salehi"})
    {
        flashback::Provider provider{};
        provider.set_name(name);
        ASSERT_NO_THROW(provider = m_database->create_provider(name));
        ASSERT_THAT(provider.id(), Gt(0));
        ASSERT_THAT(provider.name(), Eq(name));
        existing_providers.push_back(provider);
    }

    EXPECT_THAT(existing_providers, SizeIs(3));
    EXPECT_NO_THROW(matched_providers = m_database->search_providers("Doe"));
    EXPECT_THAT(matched_providers, SizeIs(2));
    ASSERT_NO_THROW(matched_providers.at(1).id());
    EXPECT_THAT(matched_providers.at(1).name(), Ne("Brian Salehi"));
    EXPECT_THAT(matched_providers.at(2).name(), Ne("Brian Salehi"));

    EXPECT_NO_THROW(matched_providers = m_database->search_providers(""));
    EXPECT_THAT(matched_providers, IsEmpty());
}

TEST_F(test_database, rename_provider)
{
    using testing::SizeIs;
    using testing::IsEmpty;

    constexpr auto modified_name{"Brian Salehi"};
    std::vector<flashback::Provider> existing_providers{};
    std::map<uint64_t, flashback::Provider> matched_providers{};
    flashback::Provider provider{};
    provider.set_name("John Doe");
    ASSERT_NO_THROW(provider = m_database->create_provider(provider.name()));
    ASSERT_THAT(provider.id(), Gt(0));
    ASSERT_THAT(provider.name(), Ne(modified_name));

    EXPECT_NO_THROW(m_database->rename_provider(provider.id(), modified_name));
    EXPECT_NO_THROW(matched_providers = m_database->search_providers("Doe"));
    EXPECT_THAT(matched_providers, IsEmpty());
    EXPECT_NO_THROW(matched_providers = m_database->search_providers("Brian"));
    EXPECT_THAT(matched_providers, SizeIs(1));
}

TEST_F(test_database, remove_provider)
{
    using testing::SizeIs;
    using testing::IsEmpty;

    std::map<uint64_t, flashback::Provider> matched_providers{};

    for (auto const& name: {"John Doe", "Jane Doe", "Brian Salehi"})
    {
        flashback::Provider provider{};
        provider.set_name(name);
        ASSERT_NO_THROW(provider = m_database->create_provider(name));
        ASSERT_THAT(provider.id(), Gt(0));
        ASSERT_THAT(provider.name(), Eq(name));
    }

    EXPECT_NO_THROW(matched_providers = m_database->search_providers("Doe"));
    EXPECT_THAT(matched_providers, SizeIs(2));
    EXPECT_NO_THROW(m_database->remove_provider(matched_providers.at(1).id()));
    EXPECT_NO_THROW(matched_providers = m_database->search_providers("Doe"));
    EXPECT_THAT(matched_providers, SizeIs(1));
}

TEST_F(test_database, merge_providers)
{
    using testing::SizeIs;
    using testing::IsEmpty;

    std::map<uint64_t, flashback::Provider> matched_providers{};

    for (auto const& name: {"John Doe", "Jane Doe"})
    {
        flashback::Provider provider{};
        provider.set_name(name);
        ASSERT_NO_THROW(provider = m_database->create_provider(name));
        ASSERT_THAT(provider.id(), Gt(0));
        ASSERT_THAT(provider.name(), Eq(name));
    }

    ASSERT_NO_THROW(matched_providers = m_database->search_providers("Doe"));
    ASSERT_THAT(matched_providers, SizeIs(2));
    ASSERT_NO_THROW(matched_providers.at(1).id());
    ASSERT_NO_THROW(matched_providers.at(2).id());
    EXPECT_NO_THROW(m_database->merge_providers(matched_providers.at(1).id(), matched_providers.at(2).id()));
    EXPECT_NO_THROW(matched_providers = m_database->search_providers("Doe"));
    EXPECT_THAT(matched_providers, SizeIs(1));
}

TEST_F(test_database, search_presenters)
{
    using testing::SizeIs;
    using testing::IsEmpty;

    std::vector<flashback::Presenter> existing_presenters{};
    std::map<uint64_t, flashback::Presenter> matched_presenters{};

    for (auto const& name: {"John Doe", "Jane Doe", "Brian Salehi"})
    {
        flashback::Presenter presenter{};
        presenter.set_name(name);
        ASSERT_NO_THROW(presenter = m_database->create_presenter(name));
        ASSERT_THAT(presenter.id(), Gt(0));
        ASSERT_THAT(presenter.name(), Eq(name));
        existing_presenters.push_back(presenter);
    }

    EXPECT_THAT(existing_presenters, SizeIs(3));
    EXPECT_NO_THROW(matched_presenters = m_database->search_presenters("Doe"));
    EXPECT_THAT(matched_presenters, SizeIs(2));
    ASSERT_NO_THROW(matched_presenters.at(1).id());
    EXPECT_THAT(matched_presenters.at(1).name(), Ne("Brian Salehi"));
    EXPECT_THAT(matched_presenters.at(2).name(), Ne("Brian Salehi"));

    EXPECT_NO_THROW(matched_presenters = m_database->search_presenters(""));
    EXPECT_THAT(matched_presenters, IsEmpty());
}

TEST_F(test_database, rename_presenter)
{
    using testing::SizeIs;
    using testing::IsEmpty;

    constexpr auto modified_name{"Brian Salehi"};
    std::vector<flashback::Presenter> existing_presenters{};
    std::map<uint64_t, flashback::Presenter> matched_presenters{};
    flashback::Presenter presenter{};
    presenter.set_name("John Doe");
    ASSERT_NO_THROW(presenter = m_database->create_presenter(presenter.name()));
    ASSERT_THAT(presenter.id(), Gt(0));
    ASSERT_THAT(presenter.name(), Ne(modified_name));

    EXPECT_NO_THROW(m_database->rename_presenter(presenter.id(), modified_name));
    EXPECT_NO_THROW(matched_presenters = m_database->search_presenters("Doe"));
    EXPECT_THAT(matched_presenters, IsEmpty());
    EXPECT_NO_THROW(matched_presenters = m_database->search_presenters("Brian"));
    EXPECT_THAT(matched_presenters, SizeIs(1));
}

TEST_F(test_database, remove_presenter)
{
    using testing::SizeIs;
    using testing::IsEmpty;

    std::map<uint64_t, flashback::Presenter> matched_presenters{};

    for (auto const& name: {"John Doe", "Jane Doe", "Brian Salehi"})
    {
        flashback::Presenter presenter{};
        presenter.set_name(name);
        ASSERT_NO_THROW(presenter = m_database->create_presenter(name));
        ASSERT_THAT(presenter.id(), Gt(0));
        ASSERT_THAT(presenter.name(), Eq(name));
    }

    EXPECT_NO_THROW(matched_presenters = m_database->search_presenters("Doe"));
    EXPECT_THAT(matched_presenters, SizeIs(2));
    EXPECT_NO_THROW(m_database->remove_presenter(matched_presenters.at(1).id()));
    EXPECT_NO_THROW(matched_presenters = m_database->search_presenters("Doe"));
    EXPECT_THAT(matched_presenters, SizeIs(1));
}

TEST_F(test_database, merge_presenters)
{
    using testing::SizeIs;
    using testing::IsEmpty;

    std::map<uint64_t, flashback::Presenter> matched_presenters{};

    for (auto const& name: {"John Doe", "Jane Doe"})
    {
        flashback::Presenter presenter{};
        presenter.set_name(name);
        ASSERT_NO_THROW(presenter = m_database->create_presenter(name));
        ASSERT_THAT(presenter.id(), Gt(0));
        ASSERT_THAT(presenter.name(), Eq(name));
    }

    ASSERT_NO_THROW(matched_presenters = m_database->search_presenters("Doe"));
    ASSERT_THAT(matched_presenters, SizeIs(2));
    ASSERT_NO_THROW(matched_presenters.at(1).id());
    ASSERT_NO_THROW(matched_presenters.at(2).id());
    EXPECT_NO_THROW(m_database->merge_presenters(matched_presenters.at(1).id(), matched_presenters.at(2).id()));
    EXPECT_NO_THROW(matched_presenters = m_database->search_presenters("Doe"));
    EXPECT_THAT(matched_presenters, SizeIs(1));
}

TEST_F(test_database, create_nerve)
{
    flashback::Subject subject{};
    subject.set_name("C++");
    flashback::Resource resource{};
    resource.set_name("Brian's Knowledge in C++");
    auto const expiration{std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch() + std::chrono::years{4})};

    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_THAT(subject.id(), Gt(0));
    EXPECT_NO_THROW(resource = m_database->create_nerve(m_user->id(), resource.name(), subject.id(), expiration.count()));
    EXPECT_THAT(resource.id(), Gt(0));
}

TEST_F(test_database, get_nerves)
{
    using testing::SizeIs;

    flashback::Subject subject{};
    subject.set_name("C++");
    flashback::Resource resource{};
    resource.set_name("C++");
    std::vector<flashback::Resource> resources{};
    std::string const expected_name{resource.name()};
    auto const expiration{std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch() + std::chrono::years{4})};

    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_THAT(subject.id(), Gt(0));
    EXPECT_NO_THROW(resource = m_database->create_nerve(m_user->id(), resource.name(), subject.id(), expiration.count()));
    EXPECT_THAT(resource.id(), Gt(0));
    EXPECT_NO_THROW(resources = m_database->get_resources(m_user->id(), subject.id()));
    ASSERT_THAT(resources, SizeIs(1));
    ASSERT_NO_THROW(resources.at(0).id());
    EXPECT_THAT(resources.at(0).id(), resource.id());
    EXPECT_THAT(resources.at(0).name(), Eq(expected_name));
    EXPECT_THAT(resources.at(0).type(), Eq(flashback::Resource::nerve));
    EXPECT_THAT(resources.at(0).pattern(), Eq(flashback::Resource::synapse));
    EXPECT_TRUE(resources.at(0).link().empty());
    EXPECT_THAT(resources.at(0).production(), Gt(0));
    EXPECT_THAT(resources.at(0).expiration(), Eq(expiration.count()));
    EXPECT_NO_THROW(resources = m_database->get_nerves(m_user->id()));
    ASSERT_THAT(resources, SizeIs(1));
    ASSERT_NO_THROW(resources.at(0).id());
    EXPECT_THAT(resources.at(0).id(), resource.id());
    EXPECT_THAT(resources.at(0).name(), Eq(expected_name));
    EXPECT_THAT(resources.at(0).type(), Eq(flashback::Resource::nerve));
    EXPECT_THAT(resources.at(0).pattern(), Eq(flashback::Resource::synapse));
    EXPECT_TRUE(resources.at(0).link().empty());
    EXPECT_THAT(resources.at(0).production(), Gt(0));
    EXPECT_THAT(resources.at(0).expiration(), Eq(expiration.count()));
}

TEST_F(test_database, create_section)
{
    using testing::SizeIs;

    auto const production{std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())};
    auto const expiration{std::chrono::duration_cast<std::chrono::seconds>(production + std::chrono::years{4})};
    std::map<uint64_t, flashback::Section> sections{};
    flashback::Section section{};
    flashback::Resource resource{};
    resource.set_name("C++");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.clear_link();
    resource.set_production(production.count());
    resource.set_expiration(expiration.count());

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));

    for (std::string const& name: {"Chapter 1", "Chapter 2", "Chapter 3"})
    {
        section.set_name(name);
        section.clear_position();
        section.clear_link();
        EXPECT_NO_THROW(section = m_database->create_section(resource.id(), section.position(), section.name(), section.link()));
        EXPECT_THAT(section.position(), Gt(0));
        EXPECT_THAT(section.name(), Eq(name));
        EXPECT_TRUE(section.link().empty());
    }
}

TEST_F(test_database, get_sections)
{
    using testing::SizeIs;

    auto const production{std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())};
    auto const expiration{std::chrono::duration_cast<std::chrono::seconds>(production + std::chrono::years{4})};
    std::map<uint64_t, flashback::Section> sections{};
    flashback::Section section{};
    flashback::Resource resource{};
    resource.set_name("C++");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.clear_link();
    resource.set_production(production.count());
    resource.set_expiration(expiration.count());

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));

    for (std::string const& name: {"Chapter 1", "Chapter 2", "Chapter 3"})
    {
        section.set_name(name);
        section.clear_position();
        section.clear_link();
        ASSERT_NO_THROW(section = m_database->create_section(resource.id(), section.position(), section.name(), section.link()));
        ASSERT_THAT(section.position(), Gt(0));
    }

    EXPECT_NO_THROW(sections = m_database->get_sections(resource.id()));
    EXPECT_THAT(sections, SizeIs(3));
}

TEST_F(test_database, remove_section)
{
    using testing::SizeIs;

    auto const production{std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())};
    auto const expiration{std::chrono::duration_cast<std::chrono::seconds>(production + std::chrono::years{4})};
    std::map<uint64_t, flashback::Section> sections{};
    flashback::Section section{};
    flashback::Resource resource{};
    resource.set_name("C++");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.clear_link();
    resource.set_production(production.count());
    resource.set_expiration(expiration.count());

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));

    for (std::string const& name: {"Chapter 1", "Chapter 2", "Chapter 3"})
    {
        section.set_name(name);
        section.clear_position();
        section.clear_link();
        ASSERT_NO_THROW(section = m_database->create_section(resource.id(), section.position(), section.name(), section.link()));
        ASSERT_THAT(section.position(), Gt(0));
        ASSERT_THAT(section.name(), Eq(name));
    }

    EXPECT_NO_THROW(sections = m_database->get_sections(resource.id()));
    EXPECT_THAT(sections, SizeIs(3));
    EXPECT_NO_THROW(m_database->remove_section(resource.id(), section.position()));
    EXPECT_NO_THROW(sections = m_database->get_sections(resource.id()));
    EXPECT_THAT(sections, SizeIs(2));
}

TEST_F(test_database, reorder_section)
{
    using testing::SizeIs;

    auto const production{std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())};
    auto const expiration{std::chrono::duration_cast<std::chrono::seconds>(production + std::chrono::years{4})};
    std::map<uint64_t, flashback::Section> sections{};
    flashback::Section section{};
    flashback::Resource resource{};
    resource.set_name("C++");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.clear_link();
    resource.set_production(production.count());
    resource.set_expiration(expiration.count());

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));

    for (std::string const& name: {"Chapter 1", "Chapter 2", "Chapter 3"})
    {
        section.set_name(name);
        section.clear_position();
        section.clear_link();
        ASSERT_NO_THROW(section = m_database->create_section(resource.id(), section.position(), section.name(), section.link()));
        ASSERT_THAT(section.position(), Gt(0));
        ASSERT_THAT(section.name(), Eq(name));
    }

    EXPECT_NO_THROW(sections = m_database->get_sections(resource.id()));
    ASSERT_THAT(sections, SizeIs(3));
    uint64_t const source_position{sections.at(1).position()};
    std::string const source_name{sections.at(1).name()};
    uint64_t const target_position{sections.at(3).position()};
    std::string const target_name{sections.at(3).name()};
    EXPECT_NO_THROW(m_database->reorder_section(resource.id(), source_position, target_position));
    EXPECT_NO_THROW(sections = m_database->get_sections(resource.id()));
    EXPECT_THAT(sections, SizeIs(3));
    ASSERT_NO_THROW(sections.at(1).position());
    ASSERT_NO_THROW(sections.at(2).name());
    ASSERT_NO_THROW(sections.at(2).position());
    ASSERT_NO_THROW(sections.at(3).position());
    ASSERT_NO_THROW(sections.at(3).name());
    EXPECT_EQ(sections.at(1).position(), source_position) << "A B C becomes B C A";
    EXPECT_THAT(sections.at(2).name(), Eq(target_name));
    EXPECT_THAT(sections.at(3).position(), Eq(target_position));
    EXPECT_THAT(sections.at(3).name(), Eq(source_name));
}

TEST_F(test_database, merge_sections)
{
    using testing::SizeIs;

    auto const production{std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())};
    auto const expiration{std::chrono::duration_cast<std::chrono::seconds>(production + std::chrono::years{4})};
    std::map<uint64_t, flashback::Section> sections{};
    flashback::Section section{};
    flashback::Resource resource{};
    resource.set_name("C++");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.clear_link();
    resource.set_production(production.count());
    resource.set_expiration(expiration.count());

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));

    for (std::string const& name: {"Chapter 1", "Chapter 2", "Chapter 3"})
    {
        section.set_name(name);
        section.clear_position();
        section.clear_link();
        ASSERT_NO_THROW(section = m_database->create_section(resource.id(), section.position(), section.name(), section.link()));
        ASSERT_THAT(section.position(), Gt(0));
        ASSERT_THAT(section.name(), Eq(name));
    }

    EXPECT_NO_THROW(sections = m_database->get_sections(resource.id()));
    ASSERT_THAT(sections, SizeIs(3));
    ASSERT_NO_THROW(sections.at(1).position());
    ASSERT_NO_THROW(sections.at(3).position());
    uint64_t const source_position{sections.at(1).position()};
    uint64_t const target_position{sections.at(3).position()};
    EXPECT_NO_THROW(m_database->merge_sections(resource.id(), source_position, target_position));
    EXPECT_NO_THROW(sections = m_database->get_sections(resource.id()));
    ASSERT_THAT(sections, SizeIs(2));
}

TEST_F(test_database, rename_section)
{
    using testing::SizeIs;

    auto const production{std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())};
    auto const expiration{std::chrono::duration_cast<std::chrono::seconds>(production + std::chrono::years{4})};
    std::map<uint64_t, flashback::Section> sections{};
    flashback::Section section{};
    flashback::Resource resource{};
    resource.set_name("C++");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.clear_link();
    resource.set_production(production.count());
    resource.set_expiration(expiration.count());

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));

    for (std::string const& name: {"Chapter 1", "Chapter 2", "Chapter 3"})
    {
        section.set_name(name);
        section.clear_position();
        section.clear_link();
        ASSERT_NO_THROW(section = m_database->create_section(resource.id(), section.position(), section.name(), section.link()));
        ASSERT_THAT(section.position(), Gt(0));
        ASSERT_THAT(section.name(), Eq(name));
    }

    EXPECT_NO_THROW(sections = m_database->get_sections(resource.id()));
    ASSERT_THAT(sections, SizeIs(3));
    constexpr auto modified_name{"Modified Section"};
    ASSERT_NO_THROW(sections.at(1).name());
    EXPECT_THAT(sections.at(1).name(), Ne(modified_name));
    EXPECT_NO_THROW(m_database->rename_section(resource.id(), sections.at(1).position(), modified_name));
    EXPECT_NO_THROW(sections = m_database->get_sections(resource.id()));
    ASSERT_THAT(sections, SizeIs(3));
    ASSERT_NO_THROW(sections.at(1).name());
    EXPECT_THAT(sections.at(1).name(), Eq(modified_name));
}

TEST_F(test_database, move_section)
{
    using testing::SizeIs;

    auto const production{std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())};
    auto const expiration{std::chrono::duration_cast<std::chrono::seconds>(production + std::chrono::years{4})};
    std::map<uint64_t, flashback::Section> sections{};
    flashback::Section section{};
    flashback::Resource resource{};
    resource.set_name("C++");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.clear_link();
    resource.set_production(production.count());
    resource.set_expiration(expiration.count());
    flashback::Resource target_resource{};
    target_resource.set_name("Rust");
    target_resource.set_type(flashback::Resource::book);
    target_resource.set_pattern(flashback::Resource::chapter);
    target_resource.clear_link();
    target_resource.set_production(production.count());
    target_resource.set_expiration(expiration.count());

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(target_resource = m_database->create_resource(target_resource));
    ASSERT_THAT(target_resource.id(), Gt(0));

    for (std::string const& name: {"Chapter 1", "Chapter 2", "Chapter 3"})
    {
        section.set_name(name);
        section.clear_position();
        section.clear_link();
        ASSERT_NO_THROW(section = m_database->create_section(resource.id(), section.position(), section.name(), section.link()));
        ASSERT_THAT(section.position(), Gt(0));
        ASSERT_THAT(section.name(), Eq(name));
        ASSERT_NO_THROW(section = m_database->create_section(target_resource.id(), section.position(), section.name(), section.link()));
        ASSERT_THAT(section.position(), Gt(0));
        ASSERT_THAT(section.name(), Eq(name));
    }

    ASSERT_NO_THROW(sections = m_database->get_sections(resource.id()));
    ASSERT_THAT(sections, SizeIs(3)) << "All sections must appear before performing move";
    EXPECT_NO_THROW(m_database->move_section(resource.id(), 1, target_resource.id(), 4));
    ASSERT_NO_THROW(sections = m_database->get_sections(resource.id()));
    ASSERT_THAT(sections, SizeIs(2)) << "Source resource must have one section less after movement";
    ASSERT_NO_THROW(sections = m_database->get_sections(target_resource.id()));
    ASSERT_THAT(sections, SizeIs(4)) << "Target resource must have one section more after movement";
    ASSERT_NO_THROW(sections.at(4).position());
    EXPECT_EQ(sections.at(4).position(), 4) << "The last section in the list must have the last position";
    EXPECT_NO_THROW(m_database->move_section(resource.id(), 2, target_resource.id(), 1));
    ASSERT_NO_THROW(sections = m_database->get_sections(resource.id()));
    ASSERT_THAT(sections, SizeIs(1)) << "Source resource must have one section less after movement";
    ASSERT_NO_THROW(sections = m_database->get_sections(target_resource.id()));
    ASSERT_THAT(sections, SizeIs(5)) << "Target resource must have one section more after movement";
    ASSERT_NO_THROW(sections.at(5).position());
    EXPECT_EQ(sections.at(5).position(), 5) << "The last section in the list must have the last position";
}

TEST_F(test_database, search_sections)
{
    using testing::SizeIs;

    auto const production{std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())};
    auto const expiration{std::chrono::duration_cast<std::chrono::seconds>(production + std::chrono::years{4})};
    std::map<uint64_t, flashback::Section> sections{};
    flashback::Section section{};
    flashback::Resource resource{};
    resource.set_name("C++");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.clear_link();
    resource.set_production(production.count());
    resource.set_expiration(expiration.count());

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));

    for (std::string const& name: {"Chapter 1", "Chapter 2", "Chapter 3"})
    {
        section.set_name(name);
        section.clear_position();
        section.clear_link();
        ASSERT_NO_THROW(section = m_database->create_section(resource.id(), section.position(), section.name(), section.link()));
        ASSERT_THAT(section.position(), Gt(0));
        ASSERT_THAT(section.name(), Eq(name));
    }

    EXPECT_NO_THROW(sections = m_database->get_sections(resource.id()));
    ASSERT_THAT(sections, SizeIs(3));
    EXPECT_NO_THROW(sections = m_database->search_sections(resource.id(), "chapter"));
    ASSERT_THAT(sections, SizeIs(3));
    EXPECT_NO_THROW(sections = m_database->search_sections(resource.id(), "1"));
    ASSERT_THAT(sections, SizeIs(1));
}

TEST_F(test_database, edit_section_link)
{
    using testing::SizeIs;

    auto const production{std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())};
    auto const expiration{std::chrono::duration_cast<std::chrono::seconds>(production + std::chrono::years{4})};
    constexpr auto link{R"(https://example.com)"};
    std::map<uint64_t, flashback::Section> sections{};
    flashback::Section section{};
    flashback::Resource resource{};
    resource.set_name("C++");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.clear_link();
    resource.set_production(production.count());
    resource.set_expiration(expiration.count());
    section.set_name("C++ Book");
    section.clear_position();
    section.set_link(link);

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(section = m_database->create_section(resource.id(), section.position(), section.name(), section.link()));
    ASSERT_THAT(section.position(), Gt(0));
    ASSERT_THAT(section.link(), Eq(link));
    EXPECT_NO_THROW(sections = m_database->get_sections(resource.id()));
    ASSERT_THAT(sections, SizeIs(1));
    constexpr auto modified_link{"https://modified.com"};
    EXPECT_THAT(section.link(), Ne(modified_link));
    EXPECT_NO_THROW(m_database->edit_section_link(resource.id(), section.position(), modified_link));
    EXPECT_NO_THROW(sections = m_database->get_sections(resource.id()));
    ASSERT_THAT(sections, SizeIs(1));
    ASSERT_NO_THROW(sections.at(1).link());
    EXPECT_THAT(sections.at(1).link(), Eq(modified_link));
}

TEST_F(test_database, create_topic)
{
    flashback::Subject subject{};
    flashback::Topic topic{};
    constexpr auto topic_name{"Lambda Functions"};
    constexpr auto level{flashback::expertise_level::surface};

    subject.set_name("C++");
    topic.clear_position();
    topic.set_name(topic_name);
    topic.set_level(level);
    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_THAT(subject.id(), Gt(0));
    EXPECT_NO_THROW(topic = m_database->create_topic(subject.id(), topic.name(), topic.level(), topic.position()));
    EXPECT_THAT(topic.position(), Gt(0));
    EXPECT_THAT(topic.name(), Eq(topic_name));
    EXPECT_THAT(topic.level(), Eq(level));
}

TEST_F(test_database, get_topics)
{
    using testing::SizeIs;

    flashback::Subject subject{};
    flashback::Topic topic{};
    std::map<uint64_t, flashback::Topic> topics{};
    constexpr auto topic_name{"C++"};
    constexpr auto level{flashback::expertise_level::surface};
    subject.set_name(topic_name);

    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_THAT(subject.id(), Gt(0));

    for (auto const& name: {"Chrono", "Coroutines", "Reflection"})
    {
        topic.clear_position();
        topic.set_name(name);
        topic.set_level(level);
        ASSERT_NO_THROW(topic = m_database->create_topic(subject.id(), topic.name(), topic.level(), topic.position()));
        ASSERT_THAT(topic.position(), Gt(0));
    }

    EXPECT_NO_THROW(topics = m_database->get_topics(subject.id(), level));
    EXPECT_THAT(topics, SizeIs(3));
    ASSERT_NO_THROW(topics.at(1).position());
    ASSERT_NO_THROW(topics.at(2).position());
    ASSERT_NO_THROW(topics.at(3).position());
    EXPECT_THAT(topics.at(1).position(), 1);
    EXPECT_THAT(topics.at(2).position(), 2);
    EXPECT_THAT(topics.at(3).position(), 3);
}

TEST_F(test_database, reorder_topic)
{
    using testing::SizeIs;

    flashback::Subject subject{};
    flashback::Topic topic{};
    std::map<uint64_t, flashback::Topic> topics{};
    constexpr auto topic_name{"C++"};
    constexpr auto level{flashback::expertise_level::surface};
    subject.set_name(topic_name);

    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_THAT(subject.id(), Gt(0));

    for (auto const& name: {"Chrono", "Coroutines", "Reflection"})
    {
        topic.clear_position();
        topic.set_name(name);
        topic.set_level(level);
        ASSERT_NO_THROW(topic = m_database->create_topic(subject.id(), topic.name(), topic.level(), topic.position()));
        ASSERT_THAT(topic.position(), Gt(0));
    }

    EXPECT_NO_THROW(topics = m_database->get_topics(subject.id(), level));
    EXPECT_THAT(topics, SizeIs(3));
    ASSERT_NO_THROW(topics.at(1).position());
    ASSERT_NO_THROW(topics.at(2).name());
    ASSERT_NO_THROW(topics.at(3).name());
    ASSERT_NO_THROW(topics.at(3).position());
    uint64_t const source_position = topics.at(1).position();
    std::string const source_name = topics.at(1).name();
    uint64_t const target_position = topics.at(3).position();
    std::string const target_name = topics.at(3).name();
    EXPECT_NO_THROW(m_database->reorder_topic(subject.id(), topics.at(1).level(), source_position, target_position));
    EXPECT_NO_THROW(topics = m_database->get_topics(subject.id(), level));
    EXPECT_THAT(topics, SizeIs(3));
    ASSERT_NO_THROW(topics.at(1).position());
    ASSERT_NO_THROW(topics.at(2).name());
    ASSERT_NO_THROW(topics.at(3).name());
    ASSERT_NO_THROW(topics.at(3).position());
    EXPECT_THAT(topics.at(1).position(), Eq(source_position));
    EXPECT_EQ(topics.at(2).name(), target_name) << "A B C becomes B C A";
    EXPECT_THAT(topics.at(3).name(), Eq(source_name));
    EXPECT_THAT(topics.at(3).position(), Eq(target_position));
}

TEST_F(test_database, remove_topic)
{
    using testing::SizeIs;

    flashback::Subject subject{};
    flashback::Topic topic{};
    std::map<uint64_t, flashback::Topic> topics{};
    constexpr auto topic_name{"C++"};
    constexpr auto level{flashback::expertise_level::surface};
    subject.set_name(topic_name);

    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_THAT(subject.id(), Gt(0));

    for (auto const& name: {"Chrono", "Coroutines", "Reflection"})
    {
        topic.clear_position();
        topic.set_name(name);
        topic.set_level(level);
        ASSERT_NO_THROW(topic = m_database->create_topic(subject.id(), topic.name(), topic.level(), topic.position()));
        ASSERT_THAT(topic.position(), Gt(0));
    }

    EXPECT_NO_THROW(topics = m_database->get_topics(subject.id(), level));
    EXPECT_THAT(topics, SizeIs(3));
    ASSERT_NO_THROW(topics.at(1).position());
    EXPECT_THAT(topics.at(1).position(), Eq(1));
    EXPECT_NO_THROW(m_database->remove_topic(subject.id(), level, 1));
    EXPECT_NO_THROW(topics = m_database->get_topics(subject.id(), level));
    EXPECT_THAT(topics, SizeIs(2));
    ASSERT_NO_THROW(topics.at(1).name());
    EXPECT_THAT(topics.at(1).position(), Eq(1));
}

TEST_F(test_database, merge_topics)
{
    using testing::SizeIs;

    flashback::Subject subject{};
    flashback::Topic topic{};
    std::map<uint64_t, flashback::Topic> topics{};
    constexpr auto topic_name{"C++"};
    constexpr auto level{flashback::expertise_level::surface};
    subject.set_name(topic_name);

    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_THAT(subject.id(), Gt(0));

    for (auto const& name: {"Chrono", "Coroutines", "Reflection"})
    {
        topic.clear_position();
        topic.set_name(name);
        topic.set_level(level);
        ASSERT_NO_THROW(topic = m_database->create_topic(subject.id(), topic.name(), topic.level(), topic.position()));
        ASSERT_THAT(topic.position(), Gt(0));
    }

    EXPECT_NO_THROW(topics = m_database->get_topics(subject.id(), level));
    EXPECT_THAT(topics, SizeIs(3));
    ASSERT_NO_THROW(topics.at(3).name());
    std::string const target_name{topics.at(3).name()};
    EXPECT_NO_THROW(m_database->merge_topics(subject.id(), topics.at(1).level(), topics.at(1).position(), topics.at(3).position()));
    EXPECT_NO_THROW(topics = m_database->get_topics(subject.id(), level));
    EXPECT_THAT(topics, SizeIs(2));
    ASSERT_NO_THROW(topics.at(2).name());
    EXPECT_THAT(topics.at(2).name(), Eq(target_name));
}

TEST_F(test_database, rename_topic)
{
    using testing::SizeIs;

    flashback::Subject subject{};
    flashback::Topic topic{};
    std::map<uint64_t, flashback::Topic> topics{};
    constexpr auto topic_name{"C++"};
    constexpr auto level{flashback::expertise_level::surface};
    subject.set_name(topic_name);

    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_THAT(subject.id(), Gt(0));

    for (auto const& name: {"Chrono", "Coroutines", "Reflection"})
    {
        topic.clear_position();
        topic.set_name(name);
        topic.set_level(level);
        ASSERT_NO_THROW(topic = m_database->create_topic(subject.id(), topic.name(), topic.level(), topic.position()));
        ASSERT_THAT(topic.position(), Gt(0));
    }

    EXPECT_NO_THROW(topics = m_database->get_topics(subject.id(), level));
    EXPECT_THAT(topics, SizeIs(3));
    constexpr auto modified_name{"Modified Topic"};
    ASSERT_NO_THROW(topics.at(1).name());
    EXPECT_THAT(topics.at(1).name(), Ne(modified_name));
    EXPECT_NO_THROW(m_database->rename_topic(subject.id(), level, topics.at(1).position(), modified_name));
    EXPECT_NO_THROW(topics = m_database->get_topics(subject.id(), level));
    EXPECT_THAT(topics, SizeIs(3));
    ASSERT_NO_THROW(topics.at(1).name());
    EXPECT_THAT(topics.at(1).name(), Eq(modified_name));
}

TEST_F(test_database, move_topic)
{
    using testing::SizeIs;

    flashback::Subject subject{};
    flashback::Subject target_subject{};
    flashback::Topic topic{};
    std::map<uint64_t, flashback::Topic> topics{};
    constexpr auto level{flashback::expertise_level::surface};
    subject.set_name("C++");
    target_subject.set_name("Rust");

    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_THAT(subject.id(), Gt(0));
    ASSERT_NO_THROW(target_subject = m_database->create_subject(target_subject.name()));
    ASSERT_THAT(target_subject.id(), Gt(0));
    ASSERT_THAT(subject.id(), Ne(target_subject.id()));

    for (auto const& name: {"Chrono", "Coroutines", "Reflection"})
    {
        topic.clear_position();
        topic.set_name(name);
        topic.set_level(level);
        ASSERT_NO_THROW(topic = m_database->create_topic(subject.id(), topic.name(), topic.level(), topic.position()));
        ASSERT_THAT(topic.position(), Gt(0));
        ASSERT_NO_THROW(topic = m_database->create_topic(target_subject.id(), topic.name(), topic.level(), topic.position()));
        ASSERT_THAT(topic.position(), Gt(0));
    }

    EXPECT_NO_THROW(topics = m_database->get_topics(subject.id(), level));
    EXPECT_THAT(topics, SizeIs(3));
    ASSERT_NO_THROW(topics.at(1).position());
    EXPECT_NO_THROW(topics = m_database->get_topics(target_subject.id(), level));
    EXPECT_THAT(topics, SizeIs(3));
    EXPECT_NO_THROW(m_database->move_topic(subject.id(), level, 1, target_subject.id(), 4));
    EXPECT_NO_THROW(topics = m_database->get_topics(subject.id(), level));
    EXPECT_THAT(topics, SizeIs(2));
    EXPECT_NO_THROW(topics = m_database->get_topics(target_subject.id(), level));
    EXPECT_THAT(topics, SizeIs(4));
    EXPECT_NO_THROW(m_database->move_topic(subject.id(), level, 1, target_subject.id(), 1));
    EXPECT_NO_THROW(topics = m_database->get_topics(subject.id(), level));
    EXPECT_THAT(topics, SizeIs(1));
    EXPECT_NO_THROW(topics = m_database->get_topics(target_subject.id(), level));
    EXPECT_THAT(topics, SizeIs(5));
}

TEST_F(test_database, search_topics)
{
    using testing::SizeIs;

    flashback::Subject subject{};
    flashback::Topic topic{};
    std::map<uint64_t, flashback::Topic> topics{};
    constexpr auto topic_name{"C++"};
    constexpr auto level{flashback::expertise_level::surface};
    constexpr auto search_pattern{"chrono"};
    subject.set_name(topic_name);

    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_THAT(subject.id(), Gt(0));

    for (auto const& name: {"Chrono", "Coroutines", "Reflection"})
    {
        topic.clear_position();
        topic.set_name(name);
        topic.set_level(level);
        ASSERT_NO_THROW(topic = m_database->create_topic(subject.id(), topic.name(), topic.level(), topic.position()));
        ASSERT_THAT(topic.position(), Gt(0));
    }

    EXPECT_NO_THROW(topics = m_database->search_topics(subject.id(), level, search_pattern));
    EXPECT_THAT(topics, SizeIs(1));
}

TEST_F(test_database, change_topic_level)
{
    using testing::SizeIs;

    flashback::Subject subject{};
    flashback::Topic topic{};
    std::map<uint64_t, flashback::Topic> topics{};
    constexpr auto topic_name{"C++"};
    constexpr auto level{flashback::expertise_level::surface};
    constexpr auto target_level{flashback::expertise_level::origin};
    subject.set_name(topic_name);

    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_THAT(subject.id(), Gt(0));

    for (auto const& name: {"Chrono", "Coroutines", "Reflection"})
    {
        topic.clear_position();
        topic.set_name(name);
        topic.set_level(level);
        ASSERT_NO_THROW(topic = m_database->create_topic(subject.id(), topic.name(), topic.level(), topic.position()));
        ASSERT_THAT(topic.position(), Gt(0));
    }

    EXPECT_NO_THROW(topics = m_database->get_topics(subject.id(), level));
    EXPECT_THAT(topics, SizeIs(3));
    ASSERT_NO_THROW(topics.at(1).level());
    EXPECT_THAT(topics.at(1).level(), Eq(level));
    EXPECT_NO_THROW(m_database->change_topic_level(subject.id(), topics.at(1).position(), topics.at(1).level(), target_level));
    EXPECT_NO_THROW(topics = m_database->get_topics(subject.id(), level));
    EXPECT_THAT(topics, SizeIs(2)) << "Subject should have one topic less in previous level";
    EXPECT_NO_THROW(topics = m_database->get_topics(subject.id(), target_level));
    EXPECT_THAT(topics, SizeIs(1)) << "Subject should have exactly one topic in target level";
    ASSERT_NO_THROW(topics.at(1).level());
    EXPECT_THAT(topics.at(1).level(), Eq(target_level));
}

TEST_F(test_database, create_card)
{
    constexpr auto headline{"Have you considered using Flashback?"};
    flashback::Card card{};
    card.set_headline(headline);

    EXPECT_THAT(card.id(), Eq(0));
    EXPECT_THAT(card.state(), Eq(flashback::Card::draft));
    EXPECT_THAT(card.headline(), Eq(headline));
    EXPECT_NO_THROW(card = m_database->create_card(card));
    EXPECT_GT(card.id(), 0) << "Card ID must be set after creation";
    EXPECT_THAT(card.state(), Eq(flashback::Card::draft));
    EXPECT_THAT(card.headline(), Eq(headline));
}

TEST_F(test_database, add_card_to_section)
{
    flashback::Resource resource{};
    flashback::Section section{};
    flashback::Card card{};
    std::chrono::seconds const production{std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())};
    std::chrono::seconds const expiration{production + std::chrono::years{1}};
    resource.clear_id();
    resource.set_name("C++ Book");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.set_link(R"(https://flashback.eu.com)");
    resource.set_production(production.count());
    resource.set_expiration(expiration.count());
    section.clear_position();
    section.set_name("Chapter 1");
    section.set_link(R"(https://flashback.eu.com/chapter1)");
    card.clear_id();
    card.set_state(flashback::Card::draft);
    card.set_headline("Have you considered using Flashback?");

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(section = m_database->create_section(resource.id(), section.position(), section.name(), section.link()));
    ASSERT_THAT(section.position(), Gt(0));
    ASSERT_NO_THROW(card = m_database->create_card(card));
    ASSERT_THAT(card.id(), Gt(0));
    EXPECT_NO_THROW(m_database->add_card_to_section(card.id(), resource.id(), section.position()));
}

TEST_F(test_database, add_card_to_topic)
{
    flashback::Resource resource{};
    flashback::Section section{};
    flashback::Card card{};
    flashback::Subject subject{};
    flashback::Topic topic{};
    std::chrono::seconds const production{std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())};
    std::chrono::seconds const expiration{production + std::chrono::years{1}};
    resource.clear_id();
    resource.set_name("C++ Book");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.set_link(R"(https://flashback.eu.com)");
    resource.set_production(production.count());
    resource.set_expiration(expiration.count());
    section.clear_position();
    section.set_name("Chapter 1");
    section.set_link(R"(https://flashback.eu.com/chapter1)");
    card.clear_id();
    card.set_state(flashback::Card::draft);
    card.set_headline("Have you considered using Flashback?");
    subject.clear_id();
    subject.set_name("C++");
    topic.clear_position();
    topic.set_level(flashback::expertise_level::surface);
    topic.set_name("Reflection");

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(section = m_database->create_section(resource.id(), section.position(), section.name(), section.link()));
    ASSERT_THAT(section.position(), Eq(1));
    ASSERT_NO_THROW(card = m_database->create_card(card));
    ASSERT_THAT(card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_THAT(subject.id(), Gt(0));
    ASSERT_NO_THROW(topic = m_database->create_topic(subject.id(), topic.name(), topic.level(), topic.position()));
    ASSERT_THAT(topic.position(), Eq(1));
    EXPECT_NO_THROW(m_database->add_card_to_topic(card.id(), subject.id(), topic.position(), topic.level()));
    EXPECT_THROW(m_database->add_card_to_topic(card.id(), subject.id(), topic.position(), topic.level()), pqxx::unique_violation);
}

TEST_F(test_database, get_section_cards)
{
    flashback::Resource resource{};
    flashback::Section section{};
    flashback::Card card{};
    flashback::Subject subject{};
    flashback::Topic topic{};
    std::chrono::seconds const production{std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())};
    std::chrono::seconds const expiration{production + std::chrono::years{1}};
    resource.clear_id();
    resource.set_name("C++ Book");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.set_link(R"(https://flashback.eu.com)");
    resource.set_production(production.count());
    resource.set_expiration(expiration.count());
    section.clear_position();
    section.set_name("Chapter 1");
    section.set_link(R"(https://flashback.eu.com/chapter1)");
    card.clear_id();
    card.set_state(flashback::Card::draft);
    card.set_headline("Have you considered using Flashback?");
    subject.clear_id();
    subject.set_name("C++");
    topic.clear_position();
    topic.set_level(flashback::expertise_level::surface);
    topic.set_name("Reflection");

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(section = m_database->create_section(resource.id(), section.position(), section.name(), section.link()));
    ASSERT_THAT(section.position(), Eq(1));
    ASSERT_NO_THROW(card = m_database->create_card(card));
    ASSERT_THAT(card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_THAT(subject.id(), Gt(0));
    ASSERT_NO_THROW(topic = m_database->create_topic(subject.id(), topic.name(), topic.level(), topic.position()));
    ASSERT_THAT(topic.position(), Eq(1));
    ASSERT_NO_THROW(m_database->add_card_to_topic(card.id(), subject.id(), topic.position(), topic.level()));
    std::vector<flashback::Card> cards{};
    EXPECT_NO_THROW(cards = m_database->get_section_cards(resource.id(), section.position()));
    EXPECT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(cards.at(0).id());
    EXPECT_THAT(cards.at(0).id(), Eq(card.id()));
}

TEST_F(test_database, get_topic_cards)
{
    flashback::Resource resource{};
    flashback::Section section{};
    flashback::Card card{};
    flashback::Subject subject{};
    flashback::Topic topic{};
    std::chrono::seconds const production{std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())};
    std::chrono::seconds const expiration{production + std::chrono::years{1}};
    resource.clear_id();
    resource.set_name("C++ Book");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.set_link(R"(https://flashback.eu.com)");
    resource.set_production(production.count());
    resource.set_expiration(expiration.count());
    section.clear_position();
    section.set_name("Chapter 1");
    section.set_link(R"(https://flashback.eu.com/chapter1)");
    card.clear_id();
    card.set_state(flashback::Card::draft);
    card.set_headline("Have you considered using Flashback?");
    subject.clear_id();
    subject.set_name("C++");
    topic.clear_position();
    topic.set_level(flashback::expertise_level::surface);
    topic.set_name("Reflection");

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(section = m_database->create_section(resource.id(), section.position(), section.name(), section.link()));
    ASSERT_THAT(section.position(), Eq(1));
    ASSERT_NO_THROW(card = m_database->create_card(card));
    ASSERT_THAT(card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_THAT(subject.id(), Gt(0));
    ASSERT_NO_THROW(topic = m_database->create_topic(subject.id(), topic.name(), topic.level(), topic.position()));
    ASSERT_THAT(topic.position(), Eq(1));
    ASSERT_NO_THROW(m_database->add_card_to_topic(card.id(), subject.id(), topic.position(), topic.level()));
    std::vector<flashback::Card> cards{};
    EXPECT_NO_THROW(cards = m_database->get_topic_cards(subject.id(), topic.position(), topic.level()));
    EXPECT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(cards.at(0).id());
    EXPECT_THAT(cards.at(0).id(), Eq(card.id()));
}

TEST_F(test_database, edit_card_headline)
{
    flashback::Resource resource{};
    flashback::Section section{};
    flashback::Card first_card{};
    flashback::Card second_card{};
    flashback::Card third_card{};
    flashback::Subject subject{};
    flashback::Topic topic{};
    std::chrono::seconds const production{std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())};
    std::chrono::seconds const expiration{production + std::chrono::years{1}};
    resource.clear_id();
    resource.set_name("C++ Book");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.set_link(R"(https://flashback.eu.com)");
    resource.set_production(production.count());
    resource.set_expiration(expiration.count());
    section.clear_position();
    section.set_name("Chapter 1");
    section.set_link(R"(https://flashback.eu.com/chapter1)");
    first_card.clear_id();
    first_card.set_state(flashback::Card::draft);
    first_card.set_headline("Have you considered using Flashback?");
    second_card.clear_id();
    second_card.set_state(flashback::Card::draft);
    second_card.set_headline("Haven't you started using Flashback yet?");
    third_card.clear_id();
    third_card.set_state(flashback::Card::draft);
    third_card.set_headline("Aren't you tired of not achieving your goals?");
    subject.clear_id();
    subject.set_name("C++");
    topic.clear_position();
    topic.set_level(flashback::expertise_level::surface);
    topic.set_name("Reflection");

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(section = m_database->create_section(resource.id(), section.position(), section.name(), section.link()));
    ASSERT_THAT(section.position(), Gt(0));
    ASSERT_NO_THROW(first_card = m_database->create_card(first_card));
    ASSERT_THAT(first_card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(first_card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(second_card = m_database->create_card(second_card));
    ASSERT_THAT(second_card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(second_card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(third_card = m_database->create_card(third_card));
    ASSERT_THAT(third_card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(third_card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_THAT(subject.id(), Gt(0));
    ASSERT_NO_THROW(topic = m_database->create_topic(subject.id(), topic.name(), topic.level(), topic.position()));
    ASSERT_THAT(topic.position(), Eq(1));
    ASSERT_NO_THROW(m_database->add_card_to_topic(first_card.id(), subject.id(), topic.position(), topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(second_card.id(), subject.id(), topic.position(), topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(third_card.id(), subject.id(), topic.position(), topic.level()));
    std::vector<flashback::Card> cards{};
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(subject.id(), topic.position(), topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(3));
    ASSERT_NO_THROW(cards = m_database->get_section_cards(resource.id(), section.position()));
    ASSERT_THAT(cards, testing::SizeIs(3));
    constexpr auto modified_headline{"Have you tried practicing with Flashback?"};
    EXPECT_NO_THROW(m_database->edit_card_headline(second_card.id(), modified_headline));
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(subject.id(), topic.position(), topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(3));
    auto iter = std::ranges::find_if(cards, [&second_card](flashback::Card const& c) { return c.id() == second_card.id(); });
    ASSERT_THAT(iter, Ne(cards.cend()));
    ASSERT_NO_THROW(iter->id());
    EXPECT_THAT(iter->id(), Eq(second_card.id()));
    EXPECT_THAT(iter->headline(), Eq(modified_headline));
}

TEST_F(test_database, remove_card)
{
    flashback::Resource resource{};
    flashback::Section section{};
    flashback::Card first_card{};
    flashback::Card second_card{};
    flashback::Card third_card{};
    flashback::Subject subject{};
    flashback::Topic topic{};
    std::chrono::seconds const production{std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())};
    std::chrono::seconds const expiration{production + std::chrono::years{1}};
    resource.clear_id();
    resource.set_name("C++ Book");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.set_link(R"(https://flashback.eu.com)");
    resource.set_production(production.count());
    resource.set_expiration(expiration.count());
    section.clear_position();
    section.set_name("Chapter 1");
    section.set_link(R"(https://flashback.eu.com/chapter1)");
    first_card.clear_id();
    first_card.set_state(flashback::Card::draft);
    first_card.set_headline("Have you considered using Flashback?");
    second_card.clear_id();
    second_card.set_state(flashback::Card::draft);
    second_card.set_headline("Haven't you started using Flashback yet?");
    third_card.clear_id();
    third_card.set_state(flashback::Card::draft);
    third_card.set_headline("Aren't you tired of not achieving your goals?");
    subject.clear_id();
    subject.set_name("C++");
    topic.clear_position();
    topic.set_level(flashback::expertise_level::surface);
    topic.set_name("Reflection");

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(section = m_database->create_section(resource.id(), section.position(), section.name(), section.link()));
    ASSERT_THAT(section.position(), Gt(0));
    ASSERT_NO_THROW(first_card = m_database->create_card(first_card));
    ASSERT_THAT(first_card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(first_card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(second_card = m_database->create_card(second_card));
    ASSERT_THAT(second_card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(second_card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(third_card = m_database->create_card(third_card));
    ASSERT_THAT(third_card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(third_card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_THAT(subject.id(), Gt(0));
    ASSERT_NO_THROW(topic = m_database->create_topic(subject.id(), topic.name(), topic.level(), topic.position()));
    ASSERT_THAT(topic.position(), Eq(1));
    ASSERT_NO_THROW(m_database->add_card_to_topic(first_card.id(), subject.id(), topic.position(), topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(second_card.id(), subject.id(), topic.position(), topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(third_card.id(), subject.id(), topic.position(), topic.level()));
    std::vector<flashback::Card> cards{};
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(subject.id(), topic.position(), topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(3));
    ASSERT_NO_THROW(cards = m_database->get_section_cards(resource.id(), section.position()));
    ASSERT_THAT(cards, testing::SizeIs(3));
    EXPECT_NO_THROW(m_database->remove_card(second_card.id()));
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(subject.id(), topic.position(), topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(2));
    ASSERT_NO_THROW(cards = m_database->get_section_cards(resource.id(), section.position()));
    ASSERT_THAT(cards, testing::SizeIs(2));
}

TEST_F(test_database, merge_cards)
{
    flashback::Resource resource{};
    flashback::Section section{};
    flashback::Card first_card{};
    flashback::Card second_card{};
    flashback::Card third_card{};
    flashback::Subject subject{};
    flashback::Topic topic{};
    std::chrono::seconds const production{std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())};
    std::chrono::seconds const expiration{production + std::chrono::years{1}};
    resource.clear_id();
    resource.set_name("C++ Book");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.set_link(R"(https://flashback.eu.com)");
    resource.set_production(production.count());
    resource.set_expiration(expiration.count());
    section.clear_position();
    section.set_name("Chapter 1");
    section.set_link(R"(https://flashback.eu.com/chapter1)");
    first_card.clear_id();
    first_card.set_state(flashback::Card::draft);
    first_card.set_headline("Have you considered using Flashback?");
    second_card.clear_id();
    second_card.set_state(flashback::Card::draft);
    second_card.set_headline("Haven't you started using Flashback yet?");
    third_card.clear_id();
    third_card.set_state(flashback::Card::draft);
    third_card.set_headline("Aren't you tired of not achieving your goals?");
    subject.clear_id();
    subject.set_name("C++");
    topic.clear_position();
    topic.set_level(flashback::expertise_level::surface);
    topic.set_name("Reflection");

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(section = m_database->create_section(resource.id(), section.position(), section.name(), section.link()));
    ASSERT_THAT(section.position(), Gt(0));
    ASSERT_NO_THROW(first_card = m_database->create_card(first_card));
    ASSERT_THAT(first_card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(first_card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(second_card = m_database->create_card(second_card));
    ASSERT_THAT(second_card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(second_card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(third_card = m_database->create_card(third_card));
    ASSERT_THAT(third_card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(third_card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_THAT(subject.id(), Gt(0));
    ASSERT_NO_THROW(topic = m_database->create_topic(subject.id(), topic.name(), topic.level(), topic.position()));
    ASSERT_THAT(topic.position(), Eq(1));
    ASSERT_NO_THROW(m_database->add_card_to_topic(first_card.id(), subject.id(), topic.position(), topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(second_card.id(), subject.id(), topic.position(), topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(third_card.id(), subject.id(), topic.position(), topic.level()));
    std::vector<flashback::Card> cards{};
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(subject.id(), topic.position(), topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(3));
    ASSERT_NO_THROW(cards = m_database->get_section_cards(resource.id(), section.position()));
    ASSERT_THAT(cards, testing::SizeIs(3));
    EXPECT_NO_THROW(m_database->merge_cards(first_card.id(), third_card.id(), third_card.headline()));
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(subject.id(), topic.position(), topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(2));
    ASSERT_NO_THROW(cards = m_database->get_section_cards(resource.id(), section.position()));
    ASSERT_THAT(cards, testing::SizeIs(2));
    auto iter = std::ranges::find_if(cards, [&third_card](flashback::Card const& card) { return card.id() == third_card.id(); });
    ASSERT_THAT(iter, Ne(cards.cend()));
    ASSERT_NO_THROW(iter->id());
    ASSERT_THAT(iter->id(), Eq(third_card.id()));
    EXPECT_THAT(iter->headline(), Eq(third_card.headline()));
}

TEST_F(test_database, search_cards)
{
    flashback::Resource resource{};
    flashback::Section section{};
    flashback::Card first_card{};
    flashback::Card second_card{};
    flashback::Card third_card{};
    flashback::Subject subject{};
    flashback::Topic topic{};
    std::chrono::seconds const production{std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())};
    std::chrono::seconds const expiration{production + std::chrono::years{1}};
    resource.clear_id();
    resource.set_name("C++ Book");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.set_link(R"(https://flashback.eu.com)");
    resource.set_production(production.count());
    resource.set_expiration(expiration.count());
    section.clear_position();
    section.set_name("Chapter 1");
    section.set_link(R"(https://flashback.eu.com/chapter1)");
    first_card.clear_id();
    first_card.set_state(flashback::Card::draft);
    first_card.set_headline("Have you considered using Flashback?");
    second_card.clear_id();
    second_card.set_state(flashback::Card::draft);
    second_card.set_headline("Haven't you started using Flashback yet?");
    third_card.clear_id();
    third_card.set_state(flashback::Card::draft);
    third_card.set_headline("Aren't you tired of not achieving your goals?");
    subject.clear_id();
    subject.set_name("C++");
    topic.clear_position();
    topic.set_level(flashback::expertise_level::surface);
    topic.set_name("Reflection");

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(section = m_database->create_section(resource.id(), section.position(), section.name(), section.link()));
    ASSERT_THAT(section.position(), Gt(0));
    ASSERT_NO_THROW(first_card = m_database->create_card(first_card));
    ASSERT_THAT(first_card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(first_card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(second_card = m_database->create_card(second_card));
    ASSERT_THAT(second_card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(second_card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(third_card = m_database->create_card(third_card));
    ASSERT_THAT(third_card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(third_card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_THAT(subject.id(), Gt(0));
    ASSERT_NO_THROW(topic = m_database->create_topic(subject.id(), topic.name(), topic.level(), topic.position()));
    ASSERT_THAT(topic.position(), Eq(1));
    ASSERT_NO_THROW(m_database->add_card_to_topic(first_card.id(), subject.id(), topic.position(), topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(second_card.id(), subject.id(), topic.position(), topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(third_card.id(), subject.id(), topic.position(), topic.level()));
    std::vector<flashback::Card> cards{};
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(subject.id(), topic.position(), topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(3));
    ASSERT_NO_THROW(cards = m_database->get_section_cards(resource.id(), section.position()));
    ASSERT_THAT(cards, testing::SizeIs(3));
    std::map<uint64_t, flashback::Card> matched_cards{};
    EXPECT_NO_THROW(matched_cards = m_database->search_cards(subject.id(), topic.level(), "flashback"));
    EXPECT_THAT(matched_cards, testing::SizeIs(2));
    EXPECT_NO_THROW(matched_cards = m_database->search_cards(subject.id(), topic.level(), "goals"));
    EXPECT_THAT(matched_cards, testing::SizeIs(1));
    ASSERT_NO_THROW(matched_cards.at(1).id());
    EXPECT_THAT(matched_cards.at(1).id(), Eq(third_card.id()));
}

TEST_F(test_database, move_card_to_section)
{
    flashback::Resource resource{};
    flashback::Section first_section{};
    flashback::Section second_section{};
    flashback::Card first_card{};
    flashback::Card second_card{};
    flashback::Card third_card{};
    flashback::Subject subject{};
    flashback::Topic topic{};
    std::chrono::seconds const production{std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())};
    std::chrono::seconds const expiration{production + std::chrono::years{1}};
    resource.clear_id();
    resource.set_name("C++ Book");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.set_link(R"(https://flashback.eu.com)");
    resource.set_production(production.count());
    resource.set_expiration(expiration.count());
    first_section.clear_position();
    first_section.set_name("Chapter 1");
    first_section.set_link(R"(https://flashback.eu.com/chapter1)");
    second_section.clear_position();
    second_section.set_name("Chapter 2");
    second_section.set_link(R"(https://flashback.eu.com/chapter2)");
    first_card.clear_id();
    first_card.set_state(flashback::Card::draft);
    first_card.set_headline("Have you considered using Flashback?");
    second_card.clear_id();
    second_card.set_state(flashback::Card::draft);
    second_card.set_headline("Haven't you started using Flashback yet?");
    third_card.clear_id();
    third_card.set_state(flashback::Card::draft);
    third_card.set_headline("Aren't you tired of not achieving your goals?");
    subject.clear_id();
    subject.set_name("C++");
    topic.clear_position();
    topic.set_level(flashback::expertise_level::surface);
    topic.set_name("Reflection");

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(first_section = m_database->create_section(resource.id(), first_section.position(), first_section.name(), first_section.link()));
    ASSERT_THAT(first_section.position(), Gt(0));
    ASSERT_NO_THROW(second_section = m_database->create_section(resource.id(), second_section.position(), second_section.name(), second_section.link()));
    ASSERT_THAT(second_section.position(), Gt(0));
    ASSERT_NO_THROW(first_card = m_database->create_card(first_card));
    ASSERT_THAT(first_card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(first_card.id(), resource.id(), first_section.position()));
    ASSERT_NO_THROW(second_card = m_database->create_card(second_card));
    ASSERT_THAT(second_card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(second_card.id(), resource.id(), first_section.position()));
    ASSERT_NO_THROW(third_card = m_database->create_card(third_card));
    ASSERT_THAT(third_card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(third_card.id(), resource.id(), second_section.position()));
    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_THAT(subject.id(), Gt(0));
    ASSERT_NO_THROW(topic = m_database->create_topic(subject.id(), topic.name(), topic.level(), topic.position()));
    ASSERT_THAT(topic.position(), Eq(1));
    ASSERT_NO_THROW(m_database->add_card_to_topic(first_card.id(), subject.id(), topic.position(), topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(second_card.id(), subject.id(), topic.position(), topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(third_card.id(), subject.id(), topic.position(), topic.level()));
    std::vector<flashback::Card> cards{};
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(subject.id(), topic.position(), topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(3));
    ASSERT_NO_THROW(cards = m_database->get_section_cards(resource.id(), first_section.position()));
    ASSERT_THAT(cards, testing::SizeIs(2));
    ASSERT_NO_THROW(cards = m_database->get_section_cards(resource.id(), second_section.position()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    EXPECT_NO_THROW(m_database->move_card_to_section(first_card.id(), resource.id(), first_section.position(), second_section.position()));
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(subject.id(), topic.position(), topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(3));
    ASSERT_NO_THROW(cards = m_database->get_section_cards(resource.id(), first_section.position()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(cards = m_database->get_section_cards(resource.id(), second_section.position()));
    ASSERT_THAT(cards, testing::SizeIs(2));
}

TEST_F(test_database, move_card_to_topic)
{
    flashback::Resource resource{};
    flashback::Section first_section{};
    flashback::Section second_section{};
    flashback::Card first_card{};
    flashback::Card second_card{};
    flashback::Card third_card{};
    flashback::Subject first_subject{};
    flashback::Subject second_subject{};
    flashback::Topic first_topic{};
    flashback::Topic second_topic{};
    flashback::Topic third_topic{};
    std::chrono::seconds const production{std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())};
    std::chrono::seconds const expiration{production + std::chrono::years{1}};
    resource.clear_id();
    resource.set_name("C++ Book");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.set_link(R"(https://flashback.eu.com)");
    resource.set_production(production.count());
    resource.set_expiration(expiration.count());
    first_section.clear_position();
    first_section.set_name("Chapter 1");
    first_section.set_link(R"(https://flashback.eu.com/chapter1)");
    second_section.clear_position();
    second_section.set_name("Chapter 2");
    second_section.set_link(R"(https://flashback.eu.com/chapter2)");
    first_card.clear_id();
    first_card.set_state(flashback::Card::draft);
    first_card.set_headline("Have you considered using Flashback?");
    second_card.clear_id();
    second_card.set_state(flashback::Card::draft);
    second_card.set_headline("Haven't you started using Flashback yet?");
    third_card.clear_id();
    third_card.set_state(flashback::Card::draft);
    third_card.set_headline("Aren't you tired of not achieving your goals?");
    first_subject.clear_id();
    first_subject.set_name("C++");
    second_subject.clear_id();
    second_subject.set_name("Rust");
    first_topic.clear_position();
    first_topic.set_level(flashback::expertise_level::surface);
    first_topic.set_name("Reflection");
    second_topic.clear_position();
    second_topic.set_level(flashback::expertise_level::surface);
    second_topic.set_name("Threads");
    third_topic.clear_position();
    third_topic.set_level(flashback::expertise_level::surface);
    third_topic.set_name("Threads");

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(first_section = m_database->create_section(resource.id(), first_section.position(), first_section.name(), first_section.link()));
    ASSERT_THAT(first_section.position(), Gt(0));
    ASSERT_NO_THROW(second_section = m_database->create_section(resource.id(), second_section.position(), second_section.name(), second_section.link()));
    ASSERT_THAT(second_section.position(), Gt(0));
    ASSERT_NO_THROW(first_card = m_database->create_card(first_card));
    ASSERT_THAT(first_card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(first_card.id(), resource.id(), first_section.position()));
    ASSERT_NO_THROW(second_card = m_database->create_card(second_card));
    ASSERT_THAT(second_card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(second_card.id(), resource.id(), first_section.position()));
    ASSERT_NO_THROW(third_card = m_database->create_card(third_card));
    ASSERT_THAT(third_card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(third_card.id(), resource.id(), second_section.position()));
    ASSERT_NO_THROW(first_subject = m_database->create_subject(first_subject.name()));
    ASSERT_THAT(first_subject.id(), Gt(0));
    ASSERT_NO_THROW(second_subject = m_database->create_subject(second_subject.name()));
    ASSERT_THAT(second_subject.id(), Gt(0));
    ASSERT_NO_THROW(first_topic = m_database->create_topic(first_subject.id(), first_topic.name(), first_topic.level(), first_topic.position()));
    ASSERT_THAT(first_topic.position(), Eq(1));
    ASSERT_NO_THROW(second_topic = m_database->create_topic(second_subject.id(), second_topic.name(), second_topic.level(), second_topic.position()));
    ASSERT_THAT(second_topic.position(), Eq(1));
    ASSERT_NO_THROW(third_topic = m_database->create_topic(second_subject.id(), third_topic.name(), third_topic.level(), third_topic.position()));
    ASSERT_THAT(third_topic.position(), Eq(2));
    ASSERT_NO_THROW(m_database->add_card_to_topic(first_card.id(), first_subject.id(), first_topic.position(), first_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(second_card.id(), second_subject.id(), second_topic.position(), second_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(third_card.id(), second_subject.id(), third_topic.position(), third_topic.level()));
    std::vector<flashback::Card> cards{};
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(first_subject.id(), first_topic.position(), first_topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(second_subject.id(), second_topic.position(), second_topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(second_subject.id(), third_topic.position(), third_topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(cards = m_database->get_section_cards(resource.id(), first_section.position()));
    ASSERT_THAT(cards, testing::SizeIs(2));
    ASSERT_NO_THROW(cards = m_database->get_section_cards(resource.id(), second_section.position()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    EXPECT_NO_THROW(
        m_database->move_card_to_topic(first_card.id(), first_subject.id(), first_topic.position(), first_topic.level(), second_subject.id(), second_topic.position(), second_topic.
            level()));
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(first_subject.id(), first_topic.position(), first_topic.level()));
    ASSERT_THAT(cards, testing::IsEmpty());
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(second_subject.id(), second_topic.position(), second_topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(2));
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(second_subject.id(), third_topic.position(), third_topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(1));
}

TEST_F(test_database, create_block)
{
    flashback::Resource resource{};
    flashback::Section first_section{};
    flashback::Section second_section{};
    flashback::Card first_card{};
    flashback::Card second_card{};
    flashback::Card third_card{};
    flashback::Subject first_subject{};
    flashback::Subject second_subject{};
    flashback::Topic first_topic{};
    flashback::Topic second_topic{};
    flashback::Topic third_topic{};
    flashback::Block first_block{};
    flashback::Block second_block{};
    flashback::Block third_block{};
    flashback::Block fourth_block{};
    flashback::Block fifth_block{};
    std::chrono::seconds const production{std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())};
    std::chrono::seconds const expiration{production + std::chrono::years{1}};
    resource.clear_id();
    resource.set_name("C++ Book");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.set_link(R"(https://flashback.eu.com)");
    resource.set_production(production.count());
    resource.set_expiration(expiration.count());
    first_section.clear_position();
    first_section.set_name("Chapter 1");
    first_section.set_link(R"(https://flashback.eu.com/chapter1)");
    second_section.clear_position();
    second_section.set_name("Chapter 2");
    second_section.set_link(R"(https://flashback.eu.com/chapter2)");
    first_card.clear_id();
    first_card.set_state(flashback::Card::draft);
    first_card.set_headline("Have you considered using Flashback?");
    second_card.clear_id();
    second_card.set_state(flashback::Card::draft);
    second_card.set_headline("Haven't you started using Flashback yet?");
    third_card.clear_id();
    third_card.set_state(flashback::Card::draft);
    third_card.set_headline("Aren't you tired of not achieving your goals?");
    first_subject.clear_id();
    first_subject.set_name("C++");
    second_subject.clear_id();
    second_subject.set_name("Rust");
    first_topic.clear_position();
    first_topic.set_level(flashback::expertise_level::surface);
    first_topic.set_name("Reflection");
    second_topic.clear_position();
    second_topic.set_level(flashback::expertise_level::surface);
    second_topic.set_name("Threads");
    third_topic.clear_position();
    third_topic.set_level(flashback::expertise_level::surface);
    third_topic.set_name("Threads");
    first_block.set_type(flashback::Block::code);
    first_block.set_extension("cpp");
    first_block.set_metadata("/path/to/file");
    first_block.set_content("auto main() -> int { }");
    second_block.set_type(flashback::Block::text);
    second_block.set_extension("txt");
    second_block.set_metadata("hint");
    second_block.set_content("This will compile.");
    third_block.set_type(flashback::Block::text);
    third_block.set_extension("txt");
    third_block.set_metadata("warning");
    third_block.set_content("This does not do anything.");
    fourth_block.set_type(flashback::Block::image);
    fourth_block.set_extension("jpg");
    fourth_block.set_metadata("logo");
    fourth_block.set_content("https://flashback.eu.com/logo");
    fifth_block.set_type(flashback::Block::code);
    fifth_block.set_extension("py");
    fifth_block.clear_metadata();
    fifth_block.set_content("print('Flashback');");

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(first_section = m_database->create_section(resource.id(), first_section.position(), first_section.name(), first_section.link()));
    ASSERT_THAT(first_section.position(), Gt(0));
    ASSERT_NO_THROW(second_section = m_database->create_section(resource.id(), second_section.position(), second_section.name(), second_section.link()));
    ASSERT_THAT(second_section.position(), Gt(0));
    ASSERT_NO_THROW(first_card = m_database->create_card(first_card));
    ASSERT_THAT(first_card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(first_card.id(), resource.id(), first_section.position()));
    ASSERT_NO_THROW(second_card = m_database->create_card(second_card));
    ASSERT_THAT(second_card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(second_card.id(), resource.id(), first_section.position()));
    ASSERT_NO_THROW(third_card = m_database->create_card(third_card));
    ASSERT_THAT(third_card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(third_card.id(), resource.id(), second_section.position()));
    ASSERT_NO_THROW(first_subject = m_database->create_subject(first_subject.name()));
    ASSERT_THAT(first_subject.id(), Gt(0));
    ASSERT_NO_THROW(second_subject = m_database->create_subject(second_subject.name()));
    ASSERT_THAT(second_subject.id(), Gt(0));
    ASSERT_NO_THROW(first_topic = m_database->create_topic(first_subject.id(), first_topic.name(), first_topic.level(), first_topic.position()));
    ASSERT_THAT(first_topic.position(), Eq(1));
    ASSERT_NO_THROW(second_topic = m_database->create_topic(second_subject.id(), second_topic.name(), second_topic.level(), second_topic.position()));
    ASSERT_THAT(second_topic.position(), Eq(1));
    ASSERT_NO_THROW(third_topic = m_database->create_topic(second_subject.id(), third_topic.name(), third_topic.level(), third_topic.position()));
    ASSERT_THAT(third_topic.position(), Eq(2));
    ASSERT_NO_THROW(m_database->add_card_to_topic(first_card.id(), first_subject.id(), first_topic.position(), first_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(second_card.id(), second_subject.id(), second_topic.position(), second_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(third_card.id(), second_subject.id(), third_topic.position(), third_topic.level()));
    std::vector<flashback::Card> cards{};
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(first_subject.id(), first_topic.position(), first_topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(second_subject.id(), second_topic.position(), second_topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(second_subject.id(), third_topic.position(), third_topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(cards = m_database->get_section_cards(resource.id(), first_section.position()));
    ASSERT_THAT(cards, testing::SizeIs(2));
    ASSERT_NO_THROW(cards = m_database->get_section_cards(resource.id(), second_section.position()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    EXPECT_NO_THROW(first_block = m_database->create_block(first_card.id(), first_block));
    EXPECT_NO_THROW(second_block = m_database->create_block(first_card.id(), second_block));
    EXPECT_NO_THROW(third_block = m_database->create_block(first_card.id(), third_block));
    EXPECT_NO_THROW(fourth_block = m_database->create_block(second_card.id(), fourth_block));
    EXPECT_NO_THROW(fifth_block = m_database->create_block(second_card.id(), fifth_block));
    EXPECT_THAT(first_block.position(), Eq(1));
    EXPECT_THAT(second_block.position(), Eq(2));
    EXPECT_THAT(third_block.position(), Eq(3));
    EXPECT_THAT(fourth_block.position(), Eq(1));
    EXPECT_THAT(fifth_block.position(), Eq(2));
}

TEST_F(test_database, get_blocks)
{
    flashback::Resource resource{};
    flashback::Section first_section{};
    flashback::Section second_section{};
    flashback::Card first_card{};
    flashback::Card second_card{};
    flashback::Card third_card{};
    flashback::Subject first_subject{};
    flashback::Subject second_subject{};
    flashback::Topic first_topic{};
    flashback::Topic second_topic{};
    flashback::Topic third_topic{};
    flashback::Block first_block{};
    flashback::Block second_block{};
    flashback::Block third_block{};
    flashback::Block fourth_block{};
    flashback::Block fifth_block{};
    std::chrono::seconds const production{std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())};
    std::chrono::seconds const expiration{production + std::chrono::years{1}};
    resource.clear_id();
    resource.set_name("C++ Book");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.set_link(R"(https://flashback.eu.com)");
    resource.set_production(production.count());
    resource.set_expiration(expiration.count());
    first_section.clear_position();
    first_section.set_name("Chapter 1");
    first_section.set_link(R"(https://flashback.eu.com/chapter1)");
    second_section.clear_position();
    second_section.set_name("Chapter 2");
    second_section.set_link(R"(https://flashback.eu.com/chapter2)");
    first_card.clear_id();
    first_card.set_state(flashback::Card::draft);
    first_card.set_headline("Have you considered using Flashback?");
    second_card.clear_id();
    second_card.set_state(flashback::Card::draft);
    second_card.set_headline("Haven't you started using Flashback yet?");
    third_card.clear_id();
    third_card.set_state(flashback::Card::draft);
    third_card.set_headline("Aren't you tired of not achieving your goals?");
    first_subject.clear_id();
    first_subject.set_name("C++");
    second_subject.clear_id();
    second_subject.set_name("Rust");
    first_topic.clear_position();
    first_topic.set_level(flashback::expertise_level::surface);
    first_topic.set_name("Reflection");
    second_topic.clear_position();
    second_topic.set_level(flashback::expertise_level::surface);
    second_topic.set_name("Threads");
    third_topic.clear_position();
    third_topic.set_level(flashback::expertise_level::surface);
    third_topic.set_name("Threads");
    first_block.set_type(flashback::Block::code);
    first_block.set_extension("cpp");
    first_block.set_metadata("/path/to/file");
    first_block.set_content("auto main() -> int { }");
    second_block.set_type(flashback::Block::text);
    second_block.set_extension("txt");
    second_block.set_metadata("hint");
    second_block.set_content("This will compile.");
    third_block.set_type(flashback::Block::text);
    third_block.set_extension("txt");
    third_block.set_metadata("warning");
    third_block.set_content("This does not do anything.");
    fourth_block.set_type(flashback::Block::image);
    fourth_block.set_extension("jpg");
    fourth_block.set_metadata("logo");
    fourth_block.set_content("https://flashback.eu.com/logo");
    fifth_block.set_type(flashback::Block::code);
    fifth_block.set_extension("py");
    fifth_block.clear_metadata();
    fifth_block.set_content("print('Flashback');");

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(first_section = m_database->create_section(resource.id(), first_section.position(), first_section.name(), first_section.link()));
    ASSERT_THAT(first_section.position(), Gt(0));
    ASSERT_NO_THROW(second_section = m_database->create_section(resource.id(), second_section.position(), second_section.name(), second_section.link()));
    ASSERT_THAT(second_section.position(), Gt(0));
    ASSERT_NO_THROW(first_card = m_database->create_card(first_card));
    ASSERT_THAT(first_card.id(), Gt(0));
    ASSERT_NO_THROW(second_card = m_database->create_card(second_card));
    ASSERT_THAT(second_card.id(), Gt(0));
    ASSERT_NO_THROW(third_card = m_database->create_card(third_card));
    ASSERT_THAT(third_card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(first_card.id(), resource.id(), first_section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_section(second_card.id(), resource.id(), first_section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_section(third_card.id(), resource.id(), second_section.position()));
    ASSERT_NO_THROW(first_subject = m_database->create_subject(first_subject.name()));
    ASSERT_THAT(first_subject.id(), Gt(0));
    ASSERT_NO_THROW(second_subject = m_database->create_subject(second_subject.name()));
    ASSERT_THAT(second_subject.id(), Gt(0));
    ASSERT_NO_THROW(first_topic = m_database->create_topic(first_subject.id(), first_topic.name(), first_topic.level(), first_topic.position()));
    ASSERT_THAT(first_topic.position(), Eq(1));
    ASSERT_NO_THROW(second_topic = m_database->create_topic(second_subject.id(), second_topic.name(), second_topic.level(), second_topic.position()));
    ASSERT_THAT(second_topic.position(), Eq(1));
    ASSERT_NO_THROW(third_topic = m_database->create_topic(second_subject.id(), third_topic.name(), third_topic.level(), third_topic.position()));
    ASSERT_THAT(third_topic.position(), Eq(2));
    ASSERT_NO_THROW(m_database->add_card_to_topic(first_card.id(), first_subject.id(), first_topic.position(), first_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(second_card.id(), second_subject.id(), second_topic.position(), second_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(third_card.id(), second_subject.id(), third_topic.position(), third_topic.level()));
    std::vector<flashback::Card> cards{};
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(first_subject.id(), first_topic.position(), first_topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(second_subject.id(), second_topic.position(), second_topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(second_subject.id(), third_topic.position(), third_topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(cards = m_database->get_section_cards(resource.id(), first_section.position()));
    ASSERT_THAT(cards, testing::SizeIs(2));
    ASSERT_NO_THROW(cards = m_database->get_section_cards(resource.id(), second_section.position()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(first_block = m_database->create_block(first_card.id(), first_block));
    ASSERT_NO_THROW(second_block = m_database->create_block(first_card.id(), second_block));
    ASSERT_NO_THROW(third_block = m_database->create_block(first_card.id(), third_block));
    ASSERT_NO_THROW(fourth_block = m_database->create_block(second_card.id(), fourth_block));
    ASSERT_NO_THROW(fifth_block = m_database->create_block(second_card.id(), fifth_block));
    ASSERT_THAT(first_block.position(), Eq(1));
    ASSERT_THAT(second_block.position(), Eq(2));
    ASSERT_THAT(third_block.position(), Eq(3));
    ASSERT_THAT(fourth_block.position(), Eq(1));
    ASSERT_THAT(fifth_block.position(), Eq(2));
    std::map<uint64_t, flashback::Block> blocks{};
    EXPECT_NO_THROW(blocks = m_database->get_blocks(first_card.id()));
    EXPECT_THAT(blocks, testing::SizeIs(3));
    EXPECT_NO_THROW(blocks = m_database->get_blocks(second_card.id()));
    EXPECT_THAT(blocks, testing::SizeIs(2));
    EXPECT_NO_THROW(blocks = m_database->get_blocks(third_card.id()));
    EXPECT_THAT(blocks, testing::IsEmpty());
}

TEST_F(test_database, remove_block)
{
    flashback::Resource resource{};
    flashback::Section first_section{};
    flashback::Section second_section{};
    flashback::Card first_card{};
    flashback::Card second_card{};
    flashback::Card third_card{};
    flashback::Subject first_subject{};
    flashback::Subject second_subject{};
    flashback::Topic first_topic{};
    flashback::Topic second_topic{};
    flashback::Topic third_topic{};
    flashback::Block first_block{};
    flashback::Block second_block{};
    flashback::Block third_block{};
    flashback::Block fourth_block{};
    flashback::Block fifth_block{};
    std::chrono::seconds const production{std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())};
    std::chrono::seconds const expiration{production + std::chrono::years{1}};
    resource.clear_id();
    resource.set_name("C++ Book");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.set_link(R"(https://flashback.eu.com)");
    resource.set_production(production.count());
    resource.set_expiration(expiration.count());
    first_section.clear_position();
    first_section.set_name("Chapter 1");
    first_section.set_link(R"(https://flashback.eu.com/chapter1)");
    second_section.clear_position();
    second_section.set_name("Chapter 2");
    second_section.set_link(R"(https://flashback.eu.com/chapter2)");
    first_card.clear_id();
    first_card.set_state(flashback::Card::draft);
    first_card.set_headline("Have you considered using Flashback?");
    second_card.clear_id();
    second_card.set_state(flashback::Card::draft);
    second_card.set_headline("Haven't you started using Flashback yet?");
    third_card.clear_id();
    third_card.set_state(flashback::Card::draft);
    third_card.set_headline("Aren't you tired of not achieving your goals?");
    first_subject.clear_id();
    first_subject.set_name("C++");
    second_subject.clear_id();
    second_subject.set_name("Rust");
    first_topic.clear_position();
    first_topic.set_level(flashback::expertise_level::surface);
    first_topic.set_name("Reflection");
    second_topic.clear_position();
    second_topic.set_level(flashback::expertise_level::surface);
    second_topic.set_name("Threads");
    third_topic.clear_position();
    third_topic.set_level(flashback::expertise_level::surface);
    third_topic.set_name("Threads");
    first_block.set_type(flashback::Block::code);
    first_block.set_extension("cpp");
    first_block.set_metadata("/path/to/file");
    first_block.set_content("auto main() -> int { }");
    second_block.set_type(flashback::Block::text);
    second_block.set_extension("txt");
    second_block.set_metadata("hint");
    second_block.set_content("This will compile.");
    third_block.set_type(flashback::Block::text);
    third_block.set_extension("txt");
    third_block.set_metadata("warning");
    third_block.set_content("This does not do anything.");
    fourth_block.set_type(flashback::Block::image);
    fourth_block.set_extension("jpg");
    fourth_block.set_metadata("logo");
    fourth_block.set_content("https://flashback.eu.com/logo");
    fifth_block.set_type(flashback::Block::code);
    fifth_block.set_extension("py");
    fifth_block.clear_metadata();
    fifth_block.set_content("print('Flashback');");

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(first_section = m_database->create_section(resource.id(), first_section.position(), first_section.name(), first_section.link()));
    ASSERT_THAT(first_section.position(), Gt(0));
    ASSERT_NO_THROW(second_section = m_database->create_section(resource.id(), second_section.position(), second_section.name(), second_section.link()));
    ASSERT_THAT(second_section.position(), Gt(0));
    ASSERT_NO_THROW(first_card = m_database->create_card(first_card));
    ASSERT_THAT(first_card.id(), Gt(0));
    ASSERT_NO_THROW(second_card = m_database->create_card(second_card));
    ASSERT_THAT(second_card.id(), Gt(0));
    ASSERT_NO_THROW(third_card = m_database->create_card(third_card));
    ASSERT_THAT(third_card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(first_card.id(), resource.id(), first_section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_section(second_card.id(), resource.id(), first_section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_section(third_card.id(), resource.id(), second_section.position()));
    ASSERT_NO_THROW(first_subject = m_database->create_subject(first_subject.name()));
    ASSERT_THAT(first_subject.id(), Gt(0));
    ASSERT_NO_THROW(second_subject = m_database->create_subject(second_subject.name()));
    ASSERT_THAT(second_subject.id(), Gt(0));
    ASSERT_NO_THROW(first_topic = m_database->create_topic(first_subject.id(), first_topic.name(), first_topic.level(), first_topic.position()));
    ASSERT_THAT(first_topic.position(), Eq(1));
    ASSERT_NO_THROW(second_topic = m_database->create_topic(second_subject.id(), second_topic.name(), second_topic.level(), second_topic.position()));
    ASSERT_THAT(second_topic.position(), Eq(1));
    ASSERT_NO_THROW(third_topic = m_database->create_topic(second_subject.id(), third_topic.name(), third_topic.level(), third_topic.position()));
    ASSERT_THAT(third_topic.position(), Eq(2));
    ASSERT_NO_THROW(m_database->add_card_to_topic(first_card.id(), first_subject.id(), first_topic.position(), first_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(second_card.id(), second_subject.id(), second_topic.position(), second_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(third_card.id(), second_subject.id(), third_topic.position(), third_topic.level()));
    std::vector<flashback::Card> cards{};
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(first_subject.id(), first_topic.position(), first_topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(second_subject.id(), second_topic.position(), second_topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(second_subject.id(), third_topic.position(), third_topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(cards = m_database->get_section_cards(resource.id(), first_section.position()));
    ASSERT_THAT(cards, testing::SizeIs(2));
    ASSERT_NO_THROW(cards = m_database->get_section_cards(resource.id(), second_section.position()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(first_block = m_database->create_block(first_card.id(), first_block));
    ASSERT_NO_THROW(second_block = m_database->create_block(first_card.id(), second_block));
    ASSERT_NO_THROW(third_block = m_database->create_block(first_card.id(), third_block));
    ASSERT_NO_THROW(fourth_block = m_database->create_block(second_card.id(), fourth_block));
    ASSERT_NO_THROW(fifth_block = m_database->create_block(second_card.id(), fifth_block));
    ASSERT_THAT(first_block.position(), Eq(1));
    ASSERT_THAT(second_block.position(), Eq(2));
    ASSERT_THAT(third_block.position(), Eq(3));
    ASSERT_THAT(fourth_block.position(), Eq(1));
    ASSERT_THAT(fifth_block.position(), Eq(2));
    std::map<uint64_t, flashback::Block> blocks{};
    ASSERT_NO_THROW(blocks = m_database->get_blocks(first_card.id()));
    ASSERT_THAT(blocks, testing::SizeIs(3));
    ASSERT_NO_THROW(blocks = m_database->get_blocks(second_card.id()));
    ASSERT_THAT(blocks, testing::SizeIs(2));
    ASSERT_NO_THROW(blocks = m_database->get_blocks(third_card.id()));
    ASSERT_THAT(blocks, testing::IsEmpty());
    EXPECT_NO_THROW(m_database->remove_block(first_card.id(), second_block.position()));
    EXPECT_NO_THROW(m_database->remove_block(second_card.id(), fourth_block.position()));
    EXPECT_NO_THROW(blocks = m_database->get_blocks(first_card.id()));
    EXPECT_THAT(blocks, testing::SizeIs(2));
    ASSERT_NO_THROW(blocks.at(1).position());
    EXPECT_THAT(blocks.at(1).position(), Eq(1));
    EXPECT_THAT(blocks.at(1).content(), Eq(first_block.content()));
    ASSERT_NO_THROW(blocks.at(2).position());
    EXPECT_THAT(blocks.at(1).position(), Eq(1));
    EXPECT_NO_THROW(blocks = m_database->get_blocks(second_card.id()));
    EXPECT_THAT(blocks, testing::SizeIs(1));
    ASSERT_NO_THROW(blocks.at(1).position());
    EXPECT_THAT(blocks.at(1).position(), Eq(1));
    EXPECT_THAT(blocks.at(1).content(), Eq(fifth_block.content()));
    EXPECT_NO_THROW(blocks = m_database->get_blocks(third_card.id()));
    EXPECT_THAT(blocks, testing::IsEmpty());
}

TEST_F(test_database, edit_block_content)
{
    flashback::Resource resource{};
    flashback::Section first_section{};
    flashback::Section second_section{};
    flashback::Card first_card{};
    flashback::Card second_card{};
    flashback::Card third_card{};
    flashback::Subject first_subject{};
    flashback::Subject second_subject{};
    flashback::Topic first_topic{};
    flashback::Topic second_topic{};
    flashback::Topic third_topic{};
    flashback::Block first_block{};
    flashback::Block second_block{};
    flashback::Block third_block{};
    flashback::Block fourth_block{};
    flashback::Block fifth_block{};
    std::chrono::seconds const production{std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())};
    std::chrono::seconds const expiration{production + std::chrono::years{1}};
    resource.clear_id();
    resource.set_name("C++ Book");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.set_link(R"(https://flashback.eu.com)");
    resource.set_production(production.count());
    resource.set_expiration(expiration.count());
    first_section.clear_position();
    first_section.set_name("Chapter 1");
    first_section.set_link(R"(https://flashback.eu.com/chapter1)");
    second_section.clear_position();
    second_section.set_name("Chapter 2");
    second_section.set_link(R"(https://flashback.eu.com/chapter2)");
    first_card.clear_id();
    first_card.set_state(flashback::Card::draft);
    first_card.set_headline("Have you considered using Flashback?");
    second_card.clear_id();
    second_card.set_state(flashback::Card::draft);
    second_card.set_headline("Haven't you started using Flashback yet?");
    third_card.clear_id();
    third_card.set_state(flashback::Card::draft);
    third_card.set_headline("Aren't you tired of not achieving your goals?");
    first_subject.clear_id();
    first_subject.set_name("C++");
    second_subject.clear_id();
    second_subject.set_name("Rust");
    first_topic.clear_position();
    first_topic.set_level(flashback::expertise_level::surface);
    first_topic.set_name("Reflection");
    second_topic.clear_position();
    second_topic.set_level(flashback::expertise_level::surface);
    second_topic.set_name("Threads");
    third_topic.clear_position();
    third_topic.set_level(flashback::expertise_level::surface);
    third_topic.set_name("Threads");
    first_block.set_type(flashback::Block::code);
    first_block.set_extension("cpp");
    first_block.set_metadata("/path/to/file");
    first_block.set_content("auto main() -> int { }");
    second_block.set_type(flashback::Block::text);
    second_block.set_extension("txt");
    second_block.set_metadata("hint");
    second_block.set_content("This will compile.");
    third_block.set_type(flashback::Block::text);
    third_block.set_extension("txt");
    third_block.set_metadata("warning");
    third_block.set_content("This does not do anything.");
    fourth_block.set_type(flashback::Block::image);
    fourth_block.set_extension("jpg");
    fourth_block.set_metadata("logo");
    fourth_block.set_content("https://flashback.eu.com/logo");
    fifth_block.set_type(flashback::Block::code);
    fifth_block.set_extension("py");
    fifth_block.clear_metadata();
    fifth_block.set_content("print('Flashback');");

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(first_section = m_database->create_section(resource.id(), first_section.position(), first_section.name(), first_section.link()));
    ASSERT_THAT(first_section.position(), Gt(0));
    ASSERT_NO_THROW(second_section = m_database->create_section(resource.id(), second_section.position(), second_section.name(), second_section.link()));
    ASSERT_THAT(second_section.position(), Gt(0));
    ASSERT_NO_THROW(first_card = m_database->create_card(first_card));
    ASSERT_THAT(first_card.id(), Gt(0));
    ASSERT_NO_THROW(second_card = m_database->create_card(second_card));
    ASSERT_THAT(second_card.id(), Gt(0));
    ASSERT_NO_THROW(third_card = m_database->create_card(third_card));
    ASSERT_THAT(third_card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(first_card.id(), resource.id(), first_section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_section(second_card.id(), resource.id(), first_section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_section(third_card.id(), resource.id(), second_section.position()));
    ASSERT_NO_THROW(first_subject = m_database->create_subject(first_subject.name()));
    ASSERT_THAT(first_subject.id(), Gt(0));
    ASSERT_NO_THROW(second_subject = m_database->create_subject(second_subject.name()));
    ASSERT_THAT(second_subject.id(), Gt(0));
    ASSERT_NO_THROW(first_topic = m_database->create_topic(first_subject.id(), first_topic.name(), first_topic.level(), first_topic.position()));
    ASSERT_THAT(first_topic.position(), Eq(1));
    ASSERT_NO_THROW(second_topic = m_database->create_topic(second_subject.id(), second_topic.name(), second_topic.level(), second_topic.position()));
    ASSERT_THAT(second_topic.position(), Eq(1));
    ASSERT_NO_THROW(third_topic = m_database->create_topic(second_subject.id(), third_topic.name(), third_topic.level(), third_topic.position()));
    ASSERT_THAT(third_topic.position(), Eq(2));
    ASSERT_NO_THROW(m_database->add_card_to_topic(first_card.id(), first_subject.id(), first_topic.position(), first_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(second_card.id(), second_subject.id(), second_topic.position(), second_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(third_card.id(), second_subject.id(), third_topic.position(), third_topic.level()));
    std::vector<flashback::Card> cards{};
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(first_subject.id(), first_topic.position(), first_topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(second_subject.id(), second_topic.position(), second_topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(second_subject.id(), third_topic.position(), third_topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(cards = m_database->get_section_cards(resource.id(), first_section.position()));
    ASSERT_THAT(cards, testing::SizeIs(2));
    ASSERT_NO_THROW(cards = m_database->get_section_cards(resource.id(), second_section.position()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(first_block = m_database->create_block(first_card.id(), first_block));
    ASSERT_NO_THROW(second_block = m_database->create_block(first_card.id(), second_block));
    ASSERT_NO_THROW(third_block = m_database->create_block(first_card.id(), third_block));
    ASSERT_NO_THROW(fourth_block = m_database->create_block(second_card.id(), fourth_block));
    ASSERT_NO_THROW(fifth_block = m_database->create_block(second_card.id(), fifth_block));
    ASSERT_THAT(first_block.position(), Eq(1));
    ASSERT_THAT(second_block.position(), Eq(2));
    ASSERT_THAT(third_block.position(), Eq(3));
    ASSERT_THAT(fourth_block.position(), Eq(1));
    ASSERT_THAT(fifth_block.position(), Eq(2));
    std::map<uint64_t, flashback::Block> blocks{};
    ASSERT_NO_THROW(blocks = m_database->get_blocks(first_card.id()));
    ASSERT_THAT(blocks, testing::SizeIs(3));
    ASSERT_NO_THROW(blocks = m_database->get_blocks(second_card.id()));
    ASSERT_THAT(blocks, testing::SizeIs(2));
    ASSERT_NO_THROW(blocks = m_database->get_blocks(third_card.id()));
    ASSERT_THAT(blocks, testing::IsEmpty());
    EXPECT_NO_THROW(m_database->edit_block_content(second_card.id(), fourth_block.position(), first_block.content()));
    EXPECT_NO_THROW(blocks = m_database->get_blocks(second_card.id()));
    EXPECT_THAT(blocks, testing::SizeIs(2));
    ASSERT_NO_THROW(blocks.at(1).position());
    EXPECT_THAT(blocks.at(1).content(), Eq(first_block.content()));
    ASSERT_NO_THROW(blocks.at(2).position());
    EXPECT_THAT(blocks.at(2).content(), Eq(fifth_block.content())) << "Content of this block should be left untouched";
}

TEST_F(test_database, edit_block_type)
{
    flashback::Resource resource{};
    flashback::Section first_section{};
    flashback::Section second_section{};
    flashback::Card first_card{};
    flashback::Card second_card{};
    flashback::Card third_card{};
    flashback::Subject first_subject{};
    flashback::Subject second_subject{};
    flashback::Topic first_topic{};
    flashback::Topic second_topic{};
    flashback::Topic third_topic{};
    flashback::Block first_block{};
    flashback::Block second_block{};
    flashback::Block third_block{};
    flashback::Block fourth_block{};
    flashback::Block fifth_block{};
    std::chrono::seconds const production{std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())};
    std::chrono::seconds const expiration{production + std::chrono::years{1}};
    resource.clear_id();
    resource.set_name("C++ Book");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.set_link(R"(https://flashback.eu.com)");
    resource.set_production(production.count());
    resource.set_expiration(expiration.count());
    first_section.clear_position();
    first_section.set_name("Chapter 1");
    first_section.set_link(R"(https://flashback.eu.com/chapter1)");
    second_section.clear_position();
    second_section.set_name("Chapter 2");
    second_section.set_link(R"(https://flashback.eu.com/chapter2)");
    first_card.clear_id();
    first_card.set_state(flashback::Card::draft);
    first_card.set_headline("Have you considered using Flashback?");
    second_card.clear_id();
    second_card.set_state(flashback::Card::draft);
    second_card.set_headline("Haven't you started using Flashback yet?");
    third_card.clear_id();
    third_card.set_state(flashback::Card::draft);
    third_card.set_headline("Aren't you tired of not achieving your goals?");
    first_subject.clear_id();
    first_subject.set_name("C++");
    second_subject.clear_id();
    second_subject.set_name("Rust");
    first_topic.clear_position();
    first_topic.set_level(flashback::expertise_level::surface);
    first_topic.set_name("Reflection");
    second_topic.clear_position();
    second_topic.set_level(flashback::expertise_level::surface);
    second_topic.set_name("Threads");
    third_topic.clear_position();
    third_topic.set_level(flashback::expertise_level::surface);
    third_topic.set_name("Threads");
    first_block.set_type(flashback::Block::code);
    first_block.set_extension("cpp");
    first_block.set_metadata("/path/to/file");
    first_block.set_content("auto main() -> int { }");
    second_block.set_type(flashback::Block::text);
    second_block.set_extension("txt");
    second_block.set_metadata("hint");
    second_block.set_content("This will compile.");
    third_block.set_type(flashback::Block::text);
    third_block.set_extension("txt");
    third_block.set_metadata("warning");
    third_block.set_content("This does not do anything.");
    fourth_block.set_type(flashback::Block::image);
    fourth_block.set_extension("jpg");
    fourth_block.set_metadata("logo");
    fourth_block.set_content("https://flashback.eu.com/logo");
    fifth_block.set_type(flashback::Block::code);
    fifth_block.set_extension("py");
    fifth_block.clear_metadata();
    fifth_block.set_content("print('Flashback');");

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(first_section = m_database->create_section(resource.id(), first_section.position(), first_section.name(), first_section.link()));
    ASSERT_THAT(first_section.position(), Gt(0));
    ASSERT_NO_THROW(second_section = m_database->create_section(resource.id(), second_section.position(), second_section.name(), second_section.link()));
    ASSERT_THAT(second_section.position(), Gt(0));
    ASSERT_NO_THROW(first_card = m_database->create_card(first_card));
    ASSERT_THAT(first_card.id(), Gt(0));
    ASSERT_NO_THROW(second_card = m_database->create_card(second_card));
    ASSERT_THAT(second_card.id(), Gt(0));
    ASSERT_NO_THROW(third_card = m_database->create_card(third_card));
    ASSERT_THAT(third_card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(first_card.id(), resource.id(), first_section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_section(second_card.id(), resource.id(), first_section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_section(third_card.id(), resource.id(), second_section.position()));
    ASSERT_NO_THROW(first_subject = m_database->create_subject(first_subject.name()));
    ASSERT_THAT(first_subject.id(), Gt(0));
    ASSERT_NO_THROW(second_subject = m_database->create_subject(second_subject.name()));
    ASSERT_THAT(second_subject.id(), Gt(0));
    ASSERT_NO_THROW(first_topic = m_database->create_topic(first_subject.id(), first_topic.name(), first_topic.level(), first_topic.position()));
    ASSERT_THAT(first_topic.position(), Eq(1));
    ASSERT_NO_THROW(second_topic = m_database->create_topic(second_subject.id(), second_topic.name(), second_topic.level(), second_topic.position()));
    ASSERT_THAT(second_topic.position(), Eq(1));
    ASSERT_NO_THROW(third_topic = m_database->create_topic(second_subject.id(), third_topic.name(), third_topic.level(), third_topic.position()));
    ASSERT_THAT(third_topic.position(), Eq(2));
    ASSERT_NO_THROW(m_database->add_card_to_topic(first_card.id(), first_subject.id(), first_topic.position(), first_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(second_card.id(), second_subject.id(), second_topic.position(), second_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(third_card.id(), second_subject.id(), third_topic.position(), third_topic.level()));
    std::vector<flashback::Card> cards{};
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(first_subject.id(), first_topic.position(), first_topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(second_subject.id(), second_topic.position(), second_topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(second_subject.id(), third_topic.position(), third_topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(cards = m_database->get_section_cards(resource.id(), first_section.position()));
    ASSERT_THAT(cards, testing::SizeIs(2));
    ASSERT_NO_THROW(cards = m_database->get_section_cards(resource.id(), second_section.position()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(first_block = m_database->create_block(first_card.id(), first_block));
    ASSERT_NO_THROW(second_block = m_database->create_block(first_card.id(), second_block));
    ASSERT_NO_THROW(third_block = m_database->create_block(first_card.id(), third_block));
    ASSERT_NO_THROW(fourth_block = m_database->create_block(second_card.id(), fourth_block));
    ASSERT_NO_THROW(fifth_block = m_database->create_block(second_card.id(), fifth_block));
    ASSERT_THAT(first_block.position(), Eq(1));
    ASSERT_THAT(second_block.position(), Eq(2));
    ASSERT_THAT(third_block.position(), Eq(3));
    ASSERT_THAT(fourth_block.position(), Eq(1));
    ASSERT_THAT(fifth_block.position(), Eq(2));
    std::map<uint64_t, flashback::Block> blocks{};
    ASSERT_NO_THROW(blocks = m_database->get_blocks(first_card.id()));
    ASSERT_THAT(blocks, testing::SizeIs(3));
    ASSERT_NO_THROW(blocks = m_database->get_blocks(second_card.id()));
    ASSERT_THAT(blocks, testing::SizeIs(2));
    ASSERT_NO_THROW(blocks = m_database->get_blocks(third_card.id()));
    ASSERT_THAT(blocks, testing::IsEmpty());
    EXPECT_NO_THROW(m_database->change_block_type(second_card.id(), fourth_block.position(), first_block.type()));
    EXPECT_NO_THROW(blocks = m_database->get_blocks(second_card.id()));
    EXPECT_THAT(blocks, testing::SizeIs(2));
    ASSERT_NO_THROW(blocks.at(2).position());
    EXPECT_THAT(blocks.at(2).type(), Eq(first_block.type()));
}

TEST_F(test_database, edit_block_extension)
{
    flashback::Resource resource{};
    flashback::Section first_section{};
    flashback::Section second_section{};
    flashback::Card first_card{};
    flashback::Card second_card{};
    flashback::Card third_card{};
    flashback::Subject first_subject{};
    flashback::Subject second_subject{};
    flashback::Topic first_topic{};
    flashback::Topic second_topic{};
    flashback::Topic third_topic{};
    flashback::Block first_block{};
    flashback::Block second_block{};
    flashback::Block third_block{};
    flashback::Block fourth_block{};
    flashback::Block fifth_block{};
    std::chrono::seconds const production{std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())};
    std::chrono::seconds const expiration{production + std::chrono::years{1}};
    resource.clear_id();
    resource.set_name("C++ Book");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.set_link(R"(https://flashback.eu.com)");
    resource.set_production(production.count());
    resource.set_expiration(expiration.count());
    first_section.clear_position();
    first_section.set_name("Chapter 1");
    first_section.set_link(R"(https://flashback.eu.com/chapter1)");
    second_section.clear_position();
    second_section.set_name("Chapter 2");
    second_section.set_link(R"(https://flashback.eu.com/chapter2)");
    first_card.clear_id();
    first_card.set_state(flashback::Card::draft);
    first_card.set_headline("Have you considered using Flashback?");
    second_card.clear_id();
    second_card.set_state(flashback::Card::draft);
    second_card.set_headline("Haven't you started using Flashback yet?");
    third_card.clear_id();
    third_card.set_state(flashback::Card::draft);
    third_card.set_headline("Aren't you tired of not achieving your goals?");
    first_subject.clear_id();
    first_subject.set_name("C++");
    second_subject.clear_id();
    second_subject.set_name("Rust");
    first_topic.clear_position();
    first_topic.set_level(flashback::expertise_level::surface);
    first_topic.set_name("Reflection");
    second_topic.clear_position();
    second_topic.set_level(flashback::expertise_level::surface);
    second_topic.set_name("Threads");
    third_topic.clear_position();
    third_topic.set_level(flashback::expertise_level::surface);
    third_topic.set_name("Threads");
    first_block.set_type(flashback::Block::code);
    first_block.set_extension("cpp");
    first_block.set_metadata("/path/to/file");
    first_block.set_content("auto main() -> int { }");
    second_block.set_type(flashback::Block::text);
    second_block.set_extension("txt");
    second_block.set_metadata("hint");
    second_block.set_content("This will compile.");
    third_block.set_type(flashback::Block::text);
    third_block.set_extension("txt");
    third_block.set_metadata("warning");
    third_block.set_content("This does not do anything.");
    fourth_block.set_type(flashback::Block::image);
    fourth_block.set_extension("jpg");
    fourth_block.set_metadata("logo");
    fourth_block.set_content("https://flashback.eu.com/logo");
    fifth_block.set_type(flashback::Block::code);
    fifth_block.set_extension("py");
    fifth_block.clear_metadata();
    fifth_block.set_content("print('Flashback');");

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(first_section = m_database->create_section(resource.id(), first_section.position(), first_section.name(), first_section.link()));
    ASSERT_THAT(first_section.position(), Gt(0));
    ASSERT_NO_THROW(second_section = m_database->create_section(resource.id(), second_section.position(), second_section.name(), second_section.link()));
    ASSERT_THAT(second_section.position(), Gt(0));
    ASSERT_NO_THROW(first_card = m_database->create_card(first_card));
    ASSERT_THAT(first_card.id(), Gt(0));
    ASSERT_NO_THROW(second_card = m_database->create_card(second_card));
    ASSERT_THAT(second_card.id(), Gt(0));
    ASSERT_NO_THROW(third_card = m_database->create_card(third_card));
    ASSERT_THAT(third_card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(first_card.id(), resource.id(), first_section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_section(second_card.id(), resource.id(), first_section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_section(third_card.id(), resource.id(), second_section.position()));
    ASSERT_NO_THROW(first_subject = m_database->create_subject(first_subject.name()));
    ASSERT_THAT(first_subject.id(), Gt(0));
    ASSERT_NO_THROW(second_subject = m_database->create_subject(second_subject.name()));
    ASSERT_THAT(second_subject.id(), Gt(0));
    ASSERT_NO_THROW(first_topic = m_database->create_topic(first_subject.id(), first_topic.name(), first_topic.level(), first_topic.position()));
    ASSERT_THAT(first_topic.position(), Eq(1));
    ASSERT_NO_THROW(second_topic = m_database->create_topic(second_subject.id(), second_topic.name(), second_topic.level(), second_topic.position()));
    ASSERT_THAT(second_topic.position(), Eq(1));
    ASSERT_NO_THROW(third_topic = m_database->create_topic(second_subject.id(), third_topic.name(), third_topic.level(), third_topic.position()));
    ASSERT_THAT(third_topic.position(), Eq(2));
    ASSERT_NO_THROW(m_database->add_card_to_topic(first_card.id(), first_subject.id(), first_topic.position(), first_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(second_card.id(), second_subject.id(), second_topic.position(), second_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(third_card.id(), second_subject.id(), third_topic.position(), third_topic.level()));
    std::vector<flashback::Card> cards{};
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(first_subject.id(), first_topic.position(), first_topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(second_subject.id(), second_topic.position(), second_topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(second_subject.id(), third_topic.position(), third_topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(cards = m_database->get_section_cards(resource.id(), first_section.position()));
    ASSERT_THAT(cards, testing::SizeIs(2));
    ASSERT_NO_THROW(cards = m_database->get_section_cards(resource.id(), second_section.position()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(first_block = m_database->create_block(first_card.id(), first_block));
    ASSERT_NO_THROW(second_block = m_database->create_block(first_card.id(), second_block));
    ASSERT_NO_THROW(third_block = m_database->create_block(first_card.id(), third_block));
    ASSERT_NO_THROW(fourth_block = m_database->create_block(second_card.id(), fourth_block));
    ASSERT_NO_THROW(fifth_block = m_database->create_block(second_card.id(), fifth_block));
    ASSERT_THAT(first_block.position(), Eq(1));
    ASSERT_THAT(second_block.position(), Eq(2));
    ASSERT_THAT(third_block.position(), Eq(3));
    ASSERT_THAT(fourth_block.position(), Eq(1));
    ASSERT_THAT(fifth_block.position(), Eq(2));
    std::map<uint64_t, flashback::Block> blocks{};
    ASSERT_NO_THROW(blocks = m_database->get_blocks(first_card.id()));
    ASSERT_THAT(blocks, testing::SizeIs(3));
    ASSERT_NO_THROW(blocks = m_database->get_blocks(second_card.id()));
    ASSERT_THAT(blocks, testing::SizeIs(2));
    ASSERT_NO_THROW(blocks = m_database->get_blocks(third_card.id()));
    ASSERT_THAT(blocks, testing::IsEmpty());
    EXPECT_NO_THROW(m_database->edit_block_extension(second_card.id(), fourth_block.position(), first_block.extension()));
    EXPECT_NO_THROW(blocks = m_database->get_blocks(second_card.id()));
    EXPECT_THAT(blocks, testing::SizeIs(2));
    ASSERT_NO_THROW(blocks.at(1).position());
    EXPECT_THAT(blocks.at(1).extension(), Eq(first_block.extension()));
    ASSERT_NO_THROW(blocks.at(2).position());
    EXPECT_THAT(blocks.at(2).extension(), Eq(fifth_block.extension())) << "Type of this block should be left unchanged";
}

TEST_F(test_database, edit_block_metadata)
{
    flashback::Resource resource{};
    flashback::Section first_section{};
    flashback::Section second_section{};
    flashback::Card first_card{};
    flashback::Card second_card{};
    flashback::Card third_card{};
    flashback::Subject first_subject{};
    flashback::Subject second_subject{};
    flashback::Topic first_topic{};
    flashback::Topic second_topic{};
    flashback::Topic third_topic{};
    flashback::Block first_block{};
    flashback::Block second_block{};
    flashback::Block third_block{};
    flashback::Block fourth_block{};
    flashback::Block fifth_block{};
    std::chrono::seconds const production{std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())};
    std::chrono::seconds const expiration{production + std::chrono::years{1}};
    resource.clear_id();
    resource.set_name("C++ Book");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.set_link(R"(https://flashback.eu.com)");
    resource.set_production(production.count());
    resource.set_expiration(expiration.count());
    first_section.clear_position();
    first_section.set_name("Chapter 1");
    first_section.set_link(R"(https://flashback.eu.com/chapter1)");
    second_section.clear_position();
    second_section.set_name("Chapter 2");
    second_section.set_link(R"(https://flashback.eu.com/chapter2)");
    first_card.clear_id();
    first_card.set_state(flashback::Card::draft);
    first_card.set_headline("Have you considered using Flashback?");
    second_card.clear_id();
    second_card.set_state(flashback::Card::draft);
    second_card.set_headline("Haven't you started using Flashback yet?");
    third_card.clear_id();
    third_card.set_state(flashback::Card::draft);
    third_card.set_headline("Aren't you tired of not achieving your goals?");
    first_subject.clear_id();
    first_subject.set_name("C++");
    second_subject.clear_id();
    second_subject.set_name("Rust");
    first_topic.clear_position();
    first_topic.set_level(flashback::expertise_level::surface);
    first_topic.set_name("Reflection");
    second_topic.clear_position();
    second_topic.set_level(flashback::expertise_level::surface);
    second_topic.set_name("Threads");
    third_topic.clear_position();
    third_topic.set_level(flashback::expertise_level::surface);
    third_topic.set_name("Threads");
    first_block.set_type(flashback::Block::code);
    first_block.set_extension("cpp");
    first_block.set_metadata("/path/to/file");
    first_block.set_content("auto main() -> int { }");
    second_block.set_type(flashback::Block::text);
    second_block.set_extension("txt");
    second_block.set_metadata("hint");
    second_block.set_content("This will compile.");
    third_block.set_type(flashback::Block::text);
    third_block.set_extension("txt");
    third_block.set_metadata("warning");
    third_block.set_content("This does not do anything.");
    fourth_block.set_type(flashback::Block::image);
    fourth_block.set_extension("jpg");
    fourth_block.set_metadata("logo");
    fourth_block.set_content("https://flashback.eu.com/logo");
    fifth_block.set_type(flashback::Block::code);
    fifth_block.set_extension("py");
    fifth_block.clear_metadata();
    fifth_block.set_content("print('Flashback');");

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(first_section = m_database->create_section(resource.id(), first_section.position(), first_section.name(), first_section.link()));
    ASSERT_THAT(first_section.position(), Gt(0));
    ASSERT_NO_THROW(second_section = m_database->create_section(resource.id(), second_section.position(), second_section.name(), second_section.link()));
    ASSERT_THAT(second_section.position(), Gt(0));
    ASSERT_NO_THROW(first_card = m_database->create_card(first_card));
    ASSERT_THAT(first_card.id(), Gt(0));
    ASSERT_NO_THROW(second_card = m_database->create_card(second_card));
    ASSERT_THAT(second_card.id(), Gt(0));
    ASSERT_NO_THROW(third_card = m_database->create_card(third_card));
    ASSERT_THAT(third_card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(first_card.id(), resource.id(), first_section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_section(second_card.id(), resource.id(), first_section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_section(third_card.id(), resource.id(), second_section.position()));
    ASSERT_NO_THROW(first_subject = m_database->create_subject(first_subject.name()));
    ASSERT_THAT(first_subject.id(), Gt(0));
    ASSERT_NO_THROW(second_subject = m_database->create_subject(second_subject.name()));
    ASSERT_THAT(second_subject.id(), Gt(0));
    ASSERT_NO_THROW(first_topic = m_database->create_topic(first_subject.id(), first_topic.name(), first_topic.level(), first_topic.position()));
    ASSERT_THAT(first_topic.position(), Eq(1));
    ASSERT_NO_THROW(second_topic = m_database->create_topic(second_subject.id(), second_topic.name(), second_topic.level(), second_topic.position()));
    ASSERT_THAT(second_topic.position(), Eq(1));
    ASSERT_NO_THROW(third_topic = m_database->create_topic(second_subject.id(), third_topic.name(), third_topic.level(), third_topic.position()));
    ASSERT_THAT(third_topic.position(), Eq(2));
    ASSERT_NO_THROW(m_database->add_card_to_topic(first_card.id(), first_subject.id(), first_topic.position(), first_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(second_card.id(), second_subject.id(), second_topic.position(), second_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(third_card.id(), second_subject.id(), third_topic.position(), third_topic.level()));
    std::vector<flashback::Card> cards{};
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(first_subject.id(), first_topic.position(), first_topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(second_subject.id(), second_topic.position(), second_topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(second_subject.id(), third_topic.position(), third_topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(cards = m_database->get_section_cards(resource.id(), first_section.position()));
    ASSERT_THAT(cards, testing::SizeIs(2));
    ASSERT_NO_THROW(cards = m_database->get_section_cards(resource.id(), second_section.position()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(first_block = m_database->create_block(first_card.id(), first_block));
    ASSERT_NO_THROW(second_block = m_database->create_block(first_card.id(), second_block));
    ASSERT_NO_THROW(third_block = m_database->create_block(first_card.id(), third_block));
    ASSERT_NO_THROW(fourth_block = m_database->create_block(second_card.id(), fourth_block));
    ASSERT_NO_THROW(fifth_block = m_database->create_block(second_card.id(), fifth_block));
    ASSERT_THAT(first_block.position(), Eq(1));
    ASSERT_THAT(second_block.position(), Eq(2));
    ASSERT_THAT(third_block.position(), Eq(3));
    ASSERT_THAT(fourth_block.position(), Eq(1));
    ASSERT_THAT(fifth_block.position(), Eq(2));
    std::map<uint64_t, flashback::Block> blocks{};
    ASSERT_NO_THROW(blocks = m_database->get_blocks(first_card.id()));
    ASSERT_THAT(blocks, testing::SizeIs(3));
    ASSERT_NO_THROW(blocks = m_database->get_blocks(second_card.id()));
    ASSERT_THAT(blocks, testing::SizeIs(2));
    ASSERT_NO_THROW(blocks = m_database->get_blocks(third_card.id()));
    ASSERT_THAT(blocks, testing::IsEmpty());
    EXPECT_NO_THROW(m_database->edit_block_metadata(second_card.id(), fourth_block.position(), first_block.metadata()));
    EXPECT_NO_THROW(blocks = m_database->get_blocks(second_card.id()));
    EXPECT_THAT(blocks, testing::SizeIs(2));
    ASSERT_NO_THROW(blocks.at(1).position());
    EXPECT_THAT(blocks.at(1).metadata(), Eq(first_block.metadata()));
    ASSERT_NO_THROW(blocks.at(2).position());
    EXPECT_THAT(blocks.at(2).metadata(), Eq(fifth_block.metadata())) << "Metadata of this block should be left unchanged";
}

TEST_F(test_database, reorder_block)
{
    flashback::Resource resource{};
    flashback::Section first_section{};
    flashback::Section second_section{};
    flashback::Card first_card{};
    flashback::Card second_card{};
    flashback::Card third_card{};
    flashback::Subject first_subject{};
    flashback::Subject second_subject{};
    flashback::Topic first_topic{};
    flashback::Topic second_topic{};
    flashback::Topic third_topic{};
    flashback::Block first_block{};
    flashback::Block second_block{};
    flashback::Block third_block{};
    flashback::Block fourth_block{};
    flashback::Block fifth_block{};
    std::chrono::seconds const production{std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())};
    std::chrono::seconds const expiration{production + std::chrono::years{1}};
    resource.clear_id();
    resource.set_name("C++ Book");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.set_link(R"(https://flashback.eu.com)");
    resource.set_production(production.count());
    resource.set_expiration(expiration.count());
    first_section.clear_position();
    first_section.set_name("Chapter 1");
    first_section.set_link(R"(https://flashback.eu.com/chapter1)");
    second_section.clear_position();
    second_section.set_name("Chapter 2");
    second_section.set_link(R"(https://flashback.eu.com/chapter2)");
    first_card.clear_id();
    first_card.set_state(flashback::Card::draft);
    first_card.set_headline("Have you considered using Flashback?");
    second_card.clear_id();
    second_card.set_state(flashback::Card::draft);
    second_card.set_headline("Haven't you started using Flashback yet?");
    third_card.clear_id();
    third_card.set_state(flashback::Card::draft);
    third_card.set_headline("Aren't you tired of not achieving your goals?");
    first_subject.clear_id();
    first_subject.set_name("C++");
    second_subject.clear_id();
    second_subject.set_name("Rust");
    first_topic.clear_position();
    first_topic.set_level(flashback::expertise_level::surface);
    first_topic.set_name("Reflection");
    second_topic.clear_position();
    second_topic.set_level(flashback::expertise_level::surface);
    second_topic.set_name("Threads");
    third_topic.clear_position();
    third_topic.set_level(flashback::expertise_level::surface);
    third_topic.set_name("Threads");
    first_block.set_type(flashback::Block::code);
    first_block.set_extension("cpp");
    first_block.set_metadata("/path/to/file");
    first_block.set_content("auto main() -> int { }");
    second_block.set_type(flashback::Block::text);
    second_block.set_extension("txt");
    second_block.set_metadata("hint");
    second_block.set_content("This will compile.");
    third_block.set_type(flashback::Block::text);
    third_block.set_extension("txt");
    third_block.set_metadata("warning");
    third_block.set_content("This does not do anything.");
    fourth_block.set_type(flashback::Block::image);
    fourth_block.set_extension("jpg");
    fourth_block.set_metadata("logo");
    fourth_block.set_content("https://flashback.eu.com/logo");
    fifth_block.set_type(flashback::Block::code);
    fifth_block.set_extension("py");
    fifth_block.clear_metadata();
    fifth_block.set_content("print('Flashback');");

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(first_section = m_database->create_section(resource.id(), first_section.position(), first_section.name(), first_section.link()));
    ASSERT_THAT(first_section.position(), Gt(0));
    ASSERT_NO_THROW(second_section = m_database->create_section(resource.id(), second_section.position(), second_section.name(), second_section.link()));
    ASSERT_THAT(second_section.position(), Gt(0));
    ASSERT_NO_THROW(first_card = m_database->create_card(first_card));
    ASSERT_THAT(first_card.id(), Gt(0));
    ASSERT_NO_THROW(second_card = m_database->create_card(second_card));
    ASSERT_THAT(second_card.id(), Gt(0));
    ASSERT_NO_THROW(third_card = m_database->create_card(third_card));
    ASSERT_THAT(third_card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(first_card.id(), resource.id(), first_section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_section(second_card.id(), resource.id(), first_section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_section(third_card.id(), resource.id(), second_section.position()));
    ASSERT_NO_THROW(first_subject = m_database->create_subject(first_subject.name()));
    ASSERT_THAT(first_subject.id(), Gt(0));
    ASSERT_NO_THROW(second_subject = m_database->create_subject(second_subject.name()));
    ASSERT_THAT(second_subject.id(), Gt(0));
    ASSERT_NO_THROW(first_topic = m_database->create_topic(first_subject.id(), first_topic.name(), first_topic.level(), first_topic.position()));
    ASSERT_THAT(first_topic.position(), Eq(1));
    ASSERT_NO_THROW(second_topic = m_database->create_topic(second_subject.id(), second_topic.name(), second_topic.level(), second_topic.position()));
    ASSERT_THAT(second_topic.position(), Eq(1));
    ASSERT_NO_THROW(third_topic = m_database->create_topic(second_subject.id(), third_topic.name(), third_topic.level(), third_topic.position()));
    ASSERT_THAT(third_topic.position(), Eq(2));
    ASSERT_NO_THROW(m_database->add_card_to_topic(first_card.id(), first_subject.id(), first_topic.position(), first_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(second_card.id(), second_subject.id(), second_topic.position(), second_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(third_card.id(), second_subject.id(), third_topic.position(), third_topic.level()));
    std::vector<flashback::Card> cards{};
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(first_subject.id(), first_topic.position(), first_topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(second_subject.id(), second_topic.position(), second_topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(second_subject.id(), third_topic.position(), third_topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(cards = m_database->get_section_cards(resource.id(), first_section.position()));
    ASSERT_THAT(cards, testing::SizeIs(2));
    ASSERT_NO_THROW(cards = m_database->get_section_cards(resource.id(), second_section.position()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(first_block = m_database->create_block(first_card.id(), first_block));
    ASSERT_NO_THROW(second_block = m_database->create_block(first_card.id(), second_block));
    ASSERT_NO_THROW(third_block = m_database->create_block(first_card.id(), third_block));
    ASSERT_NO_THROW(fourth_block = m_database->create_block(second_card.id(), fourth_block));
    ASSERT_NO_THROW(fifth_block = m_database->create_block(second_card.id(), fifth_block));
    ASSERT_THAT(first_block.position(), Eq(1));
    ASSERT_THAT(second_block.position(), Eq(2));
    ASSERT_THAT(third_block.position(), Eq(3));
    ASSERT_THAT(fourth_block.position(), Eq(1));
    ASSERT_THAT(fifth_block.position(), Eq(2));
    std::map<uint64_t, flashback::Block> blocks{};
    ASSERT_NO_THROW(blocks = m_database->get_blocks(first_card.id()));
    ASSERT_THAT(blocks, testing::SizeIs(3));
    ASSERT_NO_THROW(blocks = m_database->get_blocks(second_card.id()));
    ASSERT_THAT(blocks, testing::SizeIs(2));
    ASSERT_NO_THROW(blocks = m_database->get_blocks(third_card.id()));
    ASSERT_THAT(blocks, testing::IsEmpty());
    EXPECT_NO_THROW(m_database->reorder_block(first_card.id(), first_block.position(), third_block.position()));
    EXPECT_NO_THROW(blocks = m_database->get_blocks(first_card.id()));
    EXPECT_THAT(blocks, testing::SizeIs(3));
    ASSERT_NO_THROW(blocks.at(1).position());
    ASSERT_NO_THROW(blocks.at(2).position());
    ASSERT_NO_THROW(blocks.at(3).position());
    EXPECT_THAT(blocks.at(1).position(), Eq(1));
    EXPECT_THAT(blocks.at(1).content(), Eq(second_block.content()));
    EXPECT_THAT(blocks.at(1).type(), Eq(second_block.type()));
    EXPECT_THAT(blocks.at(2).position(), Eq(2));
    EXPECT_THAT(blocks.at(2).content(), Eq(third_block.content()));
    EXPECT_THAT(blocks.at(2).type(), Eq(third_block.type()));
    EXPECT_THAT(blocks.at(3).position(), Eq(3));
    EXPECT_THAT(blocks.at(3).content(), Eq(first_block.content()));
    EXPECT_THAT(blocks.at(3).type(), Eq(first_block.type()));
}

TEST_F(test_database, merge_blocks)
{
    flashback::Resource resource{};
    flashback::Section first_section{};
    flashback::Section second_section{};
    flashback::Card first_card{};
    flashback::Card second_card{};
    flashback::Card third_card{};
    flashback::Subject first_subject{};
    flashback::Subject second_subject{};
    flashback::Topic first_topic{};
    flashback::Topic second_topic{};
    flashback::Topic third_topic{};
    flashback::Block first_block{};
    flashback::Block second_block{};
    flashback::Block third_block{};
    flashback::Block fourth_block{};
    flashback::Block fifth_block{};
    std::chrono::seconds const production{std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())};
    std::chrono::seconds const expiration{production + std::chrono::years{1}};
    resource.clear_id();
    resource.set_name("C++ Book");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.set_link(R"(https://flashback.eu.com)");
    resource.set_production(production.count());
    resource.set_expiration(expiration.count());
    first_section.clear_position();
    first_section.set_name("Chapter 1");
    first_section.set_link(R"(https://flashback.eu.com/chapter1)");
    second_section.clear_position();
    second_section.set_name("Chapter 2");
    second_section.set_link(R"(https://flashback.eu.com/chapter2)");
    first_card.clear_id();
    first_card.set_state(flashback::Card::draft);
    first_card.set_headline("Have you considered using Flashback?");
    second_card.clear_id();
    second_card.set_state(flashback::Card::draft);
    second_card.set_headline("Haven't you started using Flashback yet?");
    third_card.clear_id();
    third_card.set_state(flashback::Card::draft);
    third_card.set_headline("Aren't you tired of not achieving your goals?");
    first_subject.clear_id();
    first_subject.set_name("C++");
    second_subject.clear_id();
    second_subject.set_name("Rust");
    first_topic.clear_position();
    first_topic.set_level(flashback::expertise_level::surface);
    first_topic.set_name("Reflection");
    second_topic.clear_position();
    second_topic.set_level(flashback::expertise_level::surface);
    second_topic.set_name("Threads");
    third_topic.clear_position();
    third_topic.set_level(flashback::expertise_level::surface);
    third_topic.set_name("Threads");
    first_block.set_type(flashback::Block::code);
    first_block.set_extension("cpp");
    first_block.set_metadata("/path/to/file");
    first_block.set_content("auto main() -> int { }");
    second_block.set_type(flashback::Block::text);
    second_block.set_extension("txt");
    second_block.set_metadata("hint");
    second_block.set_content("This will compile.");
    third_block.set_type(flashback::Block::text);
    third_block.set_extension("txt");
    third_block.set_metadata("warning");
    third_block.set_content("This does not do anything.");
    fourth_block.set_type(flashback::Block::image);
    fourth_block.set_extension("jpg");
    fourth_block.set_metadata("logo");
    fourth_block.set_content("https://flashback.eu.com/logo");
    fifth_block.set_type(flashback::Block::code);
    fifth_block.set_extension("py");
    fifth_block.clear_metadata();
    fifth_block.set_content("print('Flashback');");

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(first_section = m_database->create_section(resource.id(), first_section.position(), first_section.name(), first_section.link()));
    ASSERT_THAT(first_section.position(), Gt(0));
    ASSERT_NO_THROW(second_section = m_database->create_section(resource.id(), second_section.position(), second_section.name(), second_section.link()));
    ASSERT_THAT(second_section.position(), Gt(0));
    ASSERT_NO_THROW(first_card = m_database->create_card(first_card));
    ASSERT_THAT(first_card.id(), Gt(0));
    ASSERT_NO_THROW(second_card = m_database->create_card(second_card));
    ASSERT_THAT(second_card.id(), Gt(0));
    ASSERT_NO_THROW(third_card = m_database->create_card(third_card));
    ASSERT_THAT(third_card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(first_card.id(), resource.id(), first_section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_section(second_card.id(), resource.id(), first_section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_section(third_card.id(), resource.id(), second_section.position()));
    ASSERT_NO_THROW(first_subject = m_database->create_subject(first_subject.name()));
    ASSERT_THAT(first_subject.id(), Gt(0));
    ASSERT_NO_THROW(second_subject = m_database->create_subject(second_subject.name()));
    ASSERT_THAT(second_subject.id(), Gt(0));
    ASSERT_NO_THROW(first_topic = m_database->create_topic(first_subject.id(), first_topic.name(), first_topic.level(), first_topic.position()));
    ASSERT_THAT(first_topic.position(), Eq(1));
    ASSERT_NO_THROW(second_topic = m_database->create_topic(second_subject.id(), second_topic.name(), second_topic.level(), second_topic.position()));
    ASSERT_THAT(second_topic.position(), Eq(1));
    ASSERT_NO_THROW(third_topic = m_database->create_topic(second_subject.id(), third_topic.name(), third_topic.level(), third_topic.position()));
    ASSERT_THAT(third_topic.position(), Eq(2));
    ASSERT_NO_THROW(m_database->add_card_to_topic(first_card.id(), first_subject.id(), first_topic.position(), first_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(second_card.id(), second_subject.id(), second_topic.position(), second_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(third_card.id(), second_subject.id(), third_topic.position(), third_topic.level()));
    std::vector<flashback::Card> cards{};
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(first_subject.id(), first_topic.position(), first_topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(second_subject.id(), second_topic.position(), second_topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(second_subject.id(), third_topic.position(), third_topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(cards = m_database->get_section_cards(resource.id(), first_section.position()));
    ASSERT_THAT(cards, testing::SizeIs(2));
    ASSERT_NO_THROW(cards = m_database->get_section_cards(resource.id(), second_section.position()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(first_block = m_database->create_block(first_card.id(), first_block));
    ASSERT_NO_THROW(second_block = m_database->create_block(first_card.id(), second_block));
    ASSERT_NO_THROW(third_block = m_database->create_block(first_card.id(), third_block));
    ASSERT_NO_THROW(fourth_block = m_database->create_block(second_card.id(), fourth_block));
    ASSERT_NO_THROW(fifth_block = m_database->create_block(second_card.id(), fifth_block));
    ASSERT_THAT(first_block.position(), Eq(1));
    ASSERT_THAT(second_block.position(), Eq(2));
    ASSERT_THAT(third_block.position(), Eq(3));
    ASSERT_THAT(fourth_block.position(), Eq(1));
    ASSERT_THAT(fifth_block.position(), Eq(2));
    std::map<uint64_t, flashback::Block> blocks{};
    ASSERT_NO_THROW(blocks = m_database->get_blocks(first_card.id()));
    ASSERT_THAT(blocks, testing::SizeIs(3));
    ASSERT_NO_THROW(blocks = m_database->get_blocks(second_card.id()));
    ASSERT_THAT(blocks, testing::SizeIs(2));
    ASSERT_NO_THROW(blocks = m_database->get_blocks(third_card.id()));
    ASSERT_THAT(blocks, testing::IsEmpty());
    EXPECT_NO_THROW(m_database->merge_blocks(first_card.id(), first_block.position(), third_block.position()));
    EXPECT_NO_THROW(blocks = m_database->get_blocks(first_card.id()));
    ASSERT_THAT(blocks, testing::SizeIs(2));
    ASSERT_NO_THROW(blocks.at(1).position());
    ASSERT_NO_THROW(blocks.at(2).position());
    EXPECT_THAT(blocks.at(1).position(), Eq(1));
    EXPECT_THAT(blocks.at(2).position(), Eq(2));
    std::ostringstream combined_content{};
    combined_content << first_block.content() << "\n\n" << third_block.content();
    EXPECT_THAT(blocks.at(1).content(), Eq(second_block.content()));
    EXPECT_THAT(blocks.at(2).content(), Eq(combined_content.str()));
    EXPECT_NO_THROW(m_database->merge_blocks(second_card.id(), fourth_block.position(), fifth_block.position()));
    EXPECT_NO_THROW(blocks = m_database->get_blocks(second_card.id()));
    ASSERT_THAT(blocks, testing::SizeIs(1));
    ASSERT_NO_THROW(blocks.at(1).position());
    EXPECT_NO_THROW(blocks = m_database->get_blocks(second_card.id()));
    ASSERT_THAT(blocks, testing::SizeIs(1));
    ASSERT_NO_THROW(blocks.at(1).position());
    EXPECT_THAT(blocks.at(1).position(), Eq(1));
    combined_content.str("");
    combined_content.clear();
    combined_content << fourth_block.content() << "\n\n" << fifth_block.content();
    EXPECT_THAT(blocks.at(1).content(), Eq(combined_content.str()));
}

TEST_F(test_database, split_block)
{
    flashback::Resource resource{};
    flashback::Section first_section{};
    flashback::Section second_section{};
    flashback::Card first_card{};
    flashback::Card second_card{};
    flashback::Card third_card{};
    flashback::Subject first_subject{};
    flashback::Subject second_subject{};
    flashback::Topic first_topic{};
    flashback::Topic second_topic{};
    flashback::Topic third_topic{};
    flashback::Block first_block{};
    flashback::Block second_block{};
    flashback::Block third_block{};
    flashback::Block fourth_block{};
    flashback::Block fifth_block{};
    std::chrono::seconds const production{std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())};
    std::chrono::seconds const expiration{production + std::chrono::years{1}};
    resource.clear_id();
    resource.set_name("C++ Book");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.set_link(R"(https://flashback.eu.com)");
    resource.set_production(production.count());
    resource.set_expiration(expiration.count());
    first_section.clear_position();
    first_section.set_name("Chapter 1");
    first_section.set_link(R"(https://flashback.eu.com/chapter1)");
    second_section.clear_position();
    second_section.set_name("Chapter 2");
    second_section.set_link(R"(https://flashback.eu.com/chapter2)");
    first_card.clear_id();
    first_card.set_state(flashback::Card::draft);
    first_card.set_headline("Have you considered using Flashback?");
    second_card.clear_id();
    second_card.set_state(flashback::Card::draft);
    second_card.set_headline("Haven't you started using Flashback yet?");
    third_card.clear_id();
    third_card.set_state(flashback::Card::draft);
    third_card.set_headline("Aren't you tired of not achieving your goals?");
    first_subject.clear_id();
    first_subject.set_name("C++");
    second_subject.clear_id();
    second_subject.set_name("Rust");
    first_topic.clear_position();
    first_topic.set_level(flashback::expertise_level::surface);
    first_topic.set_name("Reflection");
    second_topic.clear_position();
    second_topic.set_level(flashback::expertise_level::surface);
    second_topic.set_name("Threads");
    third_topic.clear_position();
    third_topic.set_level(flashback::expertise_level::surface);
    third_topic.set_name("Threads");
    first_block.set_type(flashback::Block::code);
    first_block.set_extension("cpp");
    first_block.set_metadata("/path/to/file");
    first_block.set_content("auto main() -> int { }");
    second_block.set_type(flashback::Block::text);
    second_block.set_extension("txt");
    second_block.set_metadata("hint");
    second_block.set_content("This will compile.");
    third_block.set_type(flashback::Block::text);
    third_block.set_extension("txt");
    third_block.set_metadata("warning");
    third_block.set_content("This does not do anything.");
    fourth_block.set_type(flashback::Block::image);
    fourth_block.set_extension("jpg");
    fourth_block.set_metadata("logo");
    fourth_block.set_content("https://flashback.eu.com/logo");
    fifth_block.set_type(flashback::Block::code);
    fifth_block.set_extension("py");
    fifth_block.clear_metadata();
    fifth_block.set_content("print('Flashback');");

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(first_section = m_database->create_section(resource.id(), first_section.position(), first_section.name(), first_section.link()));
    ASSERT_THAT(first_section.position(), Gt(0));
    ASSERT_NO_THROW(second_section = m_database->create_section(resource.id(), second_section.position(), second_section.name(), second_section.link()));
    ASSERT_THAT(second_section.position(), Gt(0));
    ASSERT_NO_THROW(first_card = m_database->create_card(first_card));
    ASSERT_THAT(first_card.id(), Gt(0));
    ASSERT_NO_THROW(second_card = m_database->create_card(second_card));
    ASSERT_THAT(second_card.id(), Gt(0));
    ASSERT_NO_THROW(third_card = m_database->create_card(third_card));
    ASSERT_THAT(third_card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(first_card.id(), resource.id(), first_section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_section(second_card.id(), resource.id(), first_section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_section(third_card.id(), resource.id(), second_section.position()));
    ASSERT_NO_THROW(first_subject = m_database->create_subject(first_subject.name()));
    ASSERT_THAT(first_subject.id(), Gt(0));
    ASSERT_NO_THROW(second_subject = m_database->create_subject(second_subject.name()));
    ASSERT_THAT(second_subject.id(), Gt(0));
    ASSERT_NO_THROW(first_topic = m_database->create_topic(first_subject.id(), first_topic.name(), first_topic.level(), first_topic.position()));
    ASSERT_THAT(first_topic.position(), Eq(1));
    ASSERT_NO_THROW(second_topic = m_database->create_topic(second_subject.id(), second_topic.name(), second_topic.level(), second_topic.position()));
    ASSERT_THAT(second_topic.position(), Eq(1));
    ASSERT_NO_THROW(third_topic = m_database->create_topic(second_subject.id(), third_topic.name(), third_topic.level(), third_topic.position()));
    ASSERT_THAT(third_topic.position(), Eq(2));
    ASSERT_NO_THROW(m_database->add_card_to_topic(first_card.id(), first_subject.id(), first_topic.position(), first_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(second_card.id(), second_subject.id(), second_topic.position(), second_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(third_card.id(), second_subject.id(), third_topic.position(), third_topic.level()));
    std::vector<flashback::Card> cards{};
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(first_subject.id(), first_topic.position(), first_topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(second_subject.id(), second_topic.position(), second_topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(second_subject.id(), third_topic.position(), third_topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(cards = m_database->get_section_cards(resource.id(), first_section.position()));
    ASSERT_THAT(cards, testing::SizeIs(2));
    ASSERT_NO_THROW(cards = m_database->get_section_cards(resource.id(), second_section.position()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    std::ostringstream split_content{};
    split_content << second_block.content() << "\n\n\n" << third_block.content();
    first_block.set_content(split_content.str());
    ASSERT_THAT(first_block.content(), Eq(split_content.str()));
    ASSERT_NO_THROW(first_block = m_database->create_block(first_card.id(), first_block));
    ASSERT_NO_THROW(second_block = m_database->create_block(first_card.id(), second_block));
    ASSERT_NO_THROW(third_block = m_database->create_block(first_card.id(), third_block));
    ASSERT_NO_THROW(fourth_block = m_database->create_block(second_card.id(), fourth_block));
    ASSERT_NO_THROW(fifth_block = m_database->create_block(second_card.id(), fifth_block));
    ASSERT_THAT(first_block.position(), Eq(1));
    ASSERT_THAT(second_block.position(), Eq(2));
    ASSERT_THAT(third_block.position(), Eq(3));
    ASSERT_THAT(fourth_block.position(), Eq(1));
    ASSERT_THAT(fifth_block.position(), Eq(2));
    std::map<uint64_t, flashback::Block> blocks{};
    ASSERT_NO_THROW(blocks = m_database->get_blocks(first_card.id()));
    ASSERT_THAT(blocks, testing::SizeIs(3));
    ASSERT_NO_THROW(blocks.at(1).content());
    ASSERT_THAT(blocks.at(1).content(), split_content.str());
    ASSERT_NO_THROW(blocks = m_database->get_blocks(second_card.id()));
    ASSERT_THAT(blocks, testing::SizeIs(2));
    ASSERT_NO_THROW(blocks = m_database->get_blocks(third_card.id()));
    ASSERT_THAT(blocks, testing::IsEmpty());
    std::map<uint64_t, flashback::Block> split_blocks{};
    EXPECT_NO_THROW(split_blocks = m_database->split_block(first_card.id(), first_block.position()));
    EXPECT_NO_THROW(blocks = m_database->get_blocks(first_card.id()));
    EXPECT_THAT(blocks, testing::SizeIs(4)) << "The card must have one more block after splitting a card in two";
    ASSERT_NO_THROW(blocks.at(1).content());
    EXPECT_THAT(blocks.at(1).content(), Eq(second_block.content()));
    EXPECT_THAT(blocks.at(2).content(), Eq(third_block.content()));
    EXPECT_THAT(blocks.at(1).content() + "\n\n" + blocks.at(2).content(), Eq(second_block.content() + "\n\n" + third_block.content()));
    ASSERT_THAT(split_blocks, SizeIs(2));
    ASSERT_NO_THROW(split_blocks.at(1).position());
    ASSERT_NO_THROW(split_blocks.at(2).position());
    ASSERT_THAT(split_blocks.at(1).position(), Eq(1));
    ASSERT_THAT(split_blocks.at(2).position(), Eq(2));
    EXPECT_THAT(split_blocks.at(1).position(), Eq(1));
    EXPECT_THAT(split_blocks.at(2).position(), Eq(2));
    EXPECT_THAT(split_blocks.at(1).content(), Eq(second_block.content())) << "Two blocks must have their content separated";
    EXPECT_THAT(split_blocks.at(2).content(), Eq(third_block.content())) << "Two blocks must have their content separated";
    ASSERT_NO_THROW(blocks = m_database->get_blocks(second_card.id()));
    ASSERT_THAT(blocks, SizeIs(2)) << "Blocks of other cards should be left untouched";
    ASSERT_NO_THROW(blocks = m_database->get_blocks(third_card.id()));
    ASSERT_THAT(blocks, IsEmpty()) << "Blocks of other cards should be left untouched";
}

TEST_F(test_database, move_block)
{
    flashback::Resource resource{};
    flashback::Section first_section{};
    flashback::Section second_section{};
    flashback::Card first_card{};
    flashback::Card second_card{};
    flashback::Card third_card{};
    flashback::Subject first_subject{};
    flashback::Subject second_subject{};
    flashback::Topic first_topic{};
    flashback::Topic second_topic{};
    flashback::Topic third_topic{};
    flashback::Block first_block{};
    flashback::Block second_block{};
    flashback::Block third_block{};
    flashback::Block fourth_block{};
    flashback::Block fifth_block{};
    std::chrono::seconds const production{std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())};
    std::chrono::seconds const expiration{production + std::chrono::years{1}};
    resource.clear_id();
    resource.set_name("C++ Book");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.set_link(R"(https://flashback.eu.com)");
    resource.set_production(production.count());
    resource.set_expiration(expiration.count());
    first_section.clear_position();
    first_section.set_name("Chapter 1");
    first_section.set_link(R"(https://flashback.eu.com/chapter1)");
    second_section.clear_position();
    second_section.set_name("Chapter 2");
    second_section.set_link(R"(https://flashback.eu.com/chapter2)");
    first_card.clear_id();
    first_card.set_state(flashback::Card::draft);
    first_card.set_headline("Have you considered using Flashback?");
    second_card.clear_id();
    second_card.set_state(flashback::Card::draft);
    second_card.set_headline("Haven't you started using Flashback yet?");
    third_card.clear_id();
    third_card.set_state(flashback::Card::draft);
    third_card.set_headline("Aren't you tired of not achieving your goals?");
    first_subject.clear_id();
    first_subject.set_name("C++");
    second_subject.clear_id();
    second_subject.set_name("Rust");
    first_topic.clear_position();
    first_topic.set_level(flashback::expertise_level::surface);
    first_topic.set_name("Reflection");
    second_topic.clear_position();
    second_topic.set_level(flashback::expertise_level::surface);
    second_topic.set_name("Threads");
    third_topic.clear_position();
    third_topic.set_level(flashback::expertise_level::surface);
    third_topic.set_name("Threads");
    first_block.set_type(flashback::Block::code);
    first_block.set_extension("cpp");
    first_block.set_metadata("/path/to/file");
    first_block.set_content("auto main() -> int { }");
    second_block.set_type(flashback::Block::text);
    second_block.set_extension("txt");
    second_block.set_metadata("hint");
    second_block.set_content("This will compile.");
    third_block.set_type(flashback::Block::text);
    third_block.set_extension("txt");
    third_block.set_metadata("warning");
    third_block.set_content("This does not do anything.");
    fourth_block.set_type(flashback::Block::image);
    fourth_block.set_extension("jpg");
    fourth_block.set_metadata("logo");
    fourth_block.set_content("https://flashback.eu.com/logo");
    fifth_block.set_type(flashback::Block::code);
    fifth_block.set_extension("py");
    fifth_block.clear_metadata();
    fifth_block.set_content("print('Flashback');");

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(first_section = m_database->create_section(resource.id(), first_section.position(), first_section.name(), first_section.link()));
    ASSERT_THAT(first_section.position(), Gt(0));
    ASSERT_NO_THROW(second_section = m_database->create_section(resource.id(), second_section.position(), second_section.name(), second_section.link()));
    ASSERT_THAT(second_section.position(), Gt(0));
    ASSERT_NO_THROW(first_card = m_database->create_card(first_card));
    ASSERT_THAT(first_card.id(), Gt(0));
    ASSERT_NO_THROW(second_card = m_database->create_card(second_card));
    ASSERT_THAT(second_card.id(), Gt(0));
    ASSERT_NO_THROW(third_card = m_database->create_card(third_card));
    ASSERT_THAT(third_card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(first_card.id(), resource.id(), first_section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_section(second_card.id(), resource.id(), first_section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_section(third_card.id(), resource.id(), second_section.position()));
    ASSERT_NO_THROW(first_subject = m_database->create_subject(first_subject.name()));
    ASSERT_THAT(first_subject.id(), Gt(0));
    ASSERT_NO_THROW(second_subject = m_database->create_subject(second_subject.name()));
    ASSERT_THAT(second_subject.id(), Gt(0));
    ASSERT_NO_THROW(first_topic = m_database->create_topic(first_subject.id(), first_topic.name(), first_topic.level(), first_topic.position()));
    ASSERT_THAT(first_topic.position(), Eq(1));
    ASSERT_NO_THROW(second_topic = m_database->create_topic(second_subject.id(), second_topic.name(), second_topic.level(), second_topic.position()));
    ASSERT_THAT(second_topic.position(), Eq(1));
    ASSERT_NO_THROW(third_topic = m_database->create_topic(second_subject.id(), third_topic.name(), third_topic.level(), third_topic.position()));
    ASSERT_THAT(third_topic.position(), Eq(2));
    ASSERT_NO_THROW(m_database->add_card_to_topic(first_card.id(), first_subject.id(), first_topic.position(), first_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(second_card.id(), second_subject.id(), second_topic.position(), second_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(third_card.id(), second_subject.id(), third_topic.position(), third_topic.level()));
    std::vector<flashback::Card> cards{};
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(first_subject.id(), first_topic.position(), first_topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(second_subject.id(), second_topic.position(), second_topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(cards = m_database->get_topic_cards(second_subject.id(), third_topic.position(), third_topic.level()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(cards = m_database->get_section_cards(resource.id(), first_section.position()));
    ASSERT_THAT(cards, testing::SizeIs(2));
    ASSERT_NO_THROW(cards = m_database->get_section_cards(resource.id(), second_section.position()));
    ASSERT_THAT(cards, testing::SizeIs(1));
    ASSERT_NO_THROW(first_block = m_database->create_block(first_card.id(), first_block));
    ASSERT_NO_THROW(second_block = m_database->create_block(first_card.id(), second_block));
    ASSERT_NO_THROW(third_block = m_database->create_block(first_card.id(), third_block));
    ASSERT_NO_THROW(fourth_block = m_database->create_block(second_card.id(), fourth_block));
    ASSERT_NO_THROW(fifth_block = m_database->create_block(second_card.id(), fifth_block));
    ASSERT_THAT(first_block.position(), Eq(1));
    ASSERT_THAT(second_block.position(), Eq(2));
    ASSERT_THAT(third_block.position(), Eq(3));
    ASSERT_THAT(fourth_block.position(), Eq(1));
    ASSERT_THAT(fifth_block.position(), Eq(2));
    std::map<uint64_t, flashback::Block> blocks{};
    ASSERT_NO_THROW(blocks = m_database->get_blocks(first_card.id()));
    ASSERT_THAT(blocks, testing::SizeIs(3));
    ASSERT_NO_THROW(blocks = m_database->get_blocks(second_card.id()));
    ASSERT_THAT(blocks, testing::SizeIs(2));
    ASSERT_NO_THROW(blocks = m_database->get_blocks(third_card.id()));
    ASSERT_THAT(blocks, testing::IsEmpty());
    EXPECT_NO_THROW(blocks = m_database->get_blocks(first_card.id()));
    EXPECT_THAT(blocks, SizeIs(3));
    EXPECT_NO_THROW(blocks = m_database->get_blocks(second_card.id()));
    EXPECT_THAT(blocks, SizeIs(2));
    EXPECT_NO_THROW(m_database->move_block(first_card.id(), 1, second_card.id(), 3));
    EXPECT_NO_THROW(blocks = m_database->get_blocks(first_card.id()));
    EXPECT_THAT(blocks, SizeIs(2));
    ASSERT_NO_THROW(blocks.at(1).position());
    ASSERT_NO_THROW(blocks.at(2).position());
    EXPECT_THAT(blocks.at(1).position(), Eq(1));
    EXPECT_THAT(blocks.at(2).position(), Eq(2));
    EXPECT_THAT(blocks.at(1).content(), Eq(second_block.content()));
    EXPECT_THAT(blocks.at(2).content(), Eq(third_block.content()));
    EXPECT_NO_THROW(blocks = m_database->get_blocks(second_card.id()));
    EXPECT_THAT(blocks, SizeIs(3));
    ASSERT_NO_THROW(blocks.at(1).position());
    ASSERT_NO_THROW(blocks.at(2).position());
    ASSERT_NO_THROW(blocks.at(3).position());
    EXPECT_THAT(blocks.at(1).position(), Eq(1));
    EXPECT_THAT(blocks.at(2).position(), Eq(2));
    EXPECT_THAT(blocks.at(3).position(), Eq(3));
    EXPECT_THAT(blocks.at(1).content(), Eq(fourth_block.content()));
    EXPECT_THAT(blocks.at(2).content(), Eq(fifth_block.content()));
    EXPECT_THAT(blocks.at(3).content(), Eq(first_block.content()));
    EXPECT_NO_THROW(m_database->move_block(first_card.id(), 2, second_card.id(), 1));
    EXPECT_NO_THROW(blocks = m_database->get_blocks(first_card.id()));
    EXPECT_THAT(blocks, SizeIs(1));
    ASSERT_NO_THROW(blocks.at(1).position());
    EXPECT_THAT(blocks.at(1).position(), Eq(1));
    EXPECT_THAT(blocks.at(1).content(), Eq(second_block.content()));
    EXPECT_NO_THROW(blocks = m_database->get_blocks(second_card.id()));
    EXPECT_THAT(blocks, SizeIs(4));
    ASSERT_NO_THROW(blocks.at(1).position());
    ASSERT_NO_THROW(blocks.at(2).position());
    ASSERT_NO_THROW(blocks.at(3).position());
    ASSERT_NO_THROW(blocks.at(4).position());
    EXPECT_THAT(blocks.at(1).position(), Eq(1));
    EXPECT_THAT(blocks.at(2).position(), Eq(2));
    EXPECT_THAT(blocks.at(3).position(), Eq(3));
    EXPECT_THAT(blocks.at(4).position(), Eq(4));
    EXPECT_THAT(blocks.at(1).content(), Eq(third_block.content()));
    EXPECT_THAT(blocks.at(2).content(), Eq(fourth_block.content()));
    EXPECT_THAT(blocks.at(3).content(), Eq(fifth_block.content()));
    EXPECT_THAT(blocks.at(4).content(), Eq(first_block.content()));
}

TEST_F(test_database, make_progress)
{
    auto constexpr roadmap_name{"C++ Software Engineer"};
    auto constexpr subject_name{"C++"};
    auto constexpr topic_name{"Reflection"};
    auto constexpr headline{"The first question?"};
    auto milestone_level{flashback::expertise_level::surface};
    auto cognitive_level{flashback::expertise_level::depth};
    auto mode{flashback::practice_mode::aggressive};
    std::chrono::seconds duration{10};
    flashback::Roadmap roadmap{};
    flashback::Subject subject{};
    flashback::Milestone milestone{};
    flashback::Topic topic{};
    flashback::Card card{};
    card.set_state(flashback::Card::draft);
    card.set_headline(headline);

    ASSERT_NO_THROW(roadmap = m_database->create_roadmap(m_user->id(), roadmap_name));
    ASSERT_THAT(roadmap.id(), Gt(0));
    ASSERT_THAT(roadmap.name(), Eq(roadmap_name));
    ASSERT_NO_THROW(subject = m_database->create_subject(subject_name));
    ASSERT_THAT(subject.id(), Gt(0));
    ASSERT_THAT(subject.name(), Eq(subject_name));
    ASSERT_NO_THROW(milestone = m_database->add_milestone(subject.id(), milestone_level, roadmap.id()));
    ASSERT_THAT(milestone.position(), Eq(1));
    ASSERT_THAT(milestone.id(), subject.id());
    ASSERT_THAT(milestone.level(), milestone_level);
    ASSERT_NO_THROW(topic = m_database->create_topic(subject.id(), topic_name, milestone_level, 0));
    ASSERT_THAT(topic.position(), Eq(1));
    ASSERT_THAT(topic.name(), topic_name);
    ASSERT_THAT(topic.level(), milestone_level);
    ASSERT_NO_THROW(card = m_database->create_card(card));
    ASSERT_THAT(card.id(), Gt(0));
    ASSERT_THAT(card.headline(), Eq(headline));
    ASSERT_NO_THROW(m_database->add_card_to_topic(card.id(), subject.id(), topic.position(), topic.level()));
    EXPECT_NO_THROW(m_database->make_progress(m_user->id(), card.id(), duration.count(), mode));
}

TEST_F(test_database, get_practice_mode)
{
    auto constexpr roadmap_name{"C++ Software Engineer"};
    auto constexpr subject_name{"C++"};
    auto constexpr topic_name{"Reflection"};
    auto constexpr first_headline{"The first question?"};
    auto constexpr second_headline{"The second question?"};
    auto constexpr third_headline{"The third question?"};
    auto milestone_level{flashback::expertise_level::surface};
    auto cognitive_level{flashback::expertise_level::depth};
    auto mode{flashback::practice_mode::aggressive};
    std::chrono::seconds duration{10};
    flashback::Roadmap roadmap{};
    flashback::Subject subject{};
    flashback::Milestone milestone{};
    flashback::Topic topic{};
    flashback::Card first_card{};
    flashback::Card second_card{};
    flashback::Card third_card{};
    first_card.set_state(flashback::Card::draft);
    first_card.set_headline(first_headline);
    second_card.set_state(flashback::Card::draft);
    second_card.set_headline(second_headline);
    third_card.set_state(flashback::Card::draft);
    third_card.set_headline(third_headline);

    ASSERT_NO_THROW(roadmap = m_database->create_roadmap(m_user->id(), roadmap_name));
    ASSERT_THAT(roadmap.id(), Gt(0));
    ASSERT_THAT(roadmap.name(), Eq(roadmap_name));
    ASSERT_NO_THROW(subject = m_database->create_subject(subject_name));
    ASSERT_THAT(subject.id(), Gt(0));
    ASSERT_THAT(subject.name(), Eq(subject_name));
    ASSERT_NO_THROW(milestone = m_database->add_milestone(subject.id(), milestone_level, roadmap.id()));
    ASSERT_THAT(milestone.position(), Eq(1));
    ASSERT_THAT(milestone.id(), subject.id());
    ASSERT_THAT(milestone.level(), milestone_level);
    ASSERT_NO_THROW(topic = m_database->create_topic(subject.id(), topic_name, milestone_level, 0));
    ASSERT_THAT(topic.position(), Eq(1));
    ASSERT_THAT(topic.name(), topic_name);
    ASSERT_THAT(topic.level(), milestone_level);
    ASSERT_NO_THROW(first_card = m_database->create_card(first_card));
    ASSERT_NO_THROW(second_card = m_database->create_card(second_card));
    ASSERT_NO_THROW(third_card = m_database->create_card(third_card));
    ASSERT_THAT(first_card.id(), Gt(0));
    ASSERT_THAT(second_card.id(), Gt(0));
    ASSERT_THAT(third_card.id(), Gt(0));
    ASSERT_THAT(first_card.headline(), Eq(first_headline));
    ASSERT_THAT(second_card.headline(), Eq(second_headline));
    ASSERT_THAT(third_card.headline(), Eq(third_headline));
    ASSERT_NO_THROW(m_database->add_card_to_topic(first_card.id(), subject.id(), topic.position(), topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(second_card.id(), subject.id(), topic.position(), topic.level()));
    EXPECT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), milestone_level));
    EXPECT_THAT(mode, Eq(flashback::practice_mode::aggressive)) << "There is no progress yet, practice mode should be aggressive";
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), first_card.id(), duration.count(), mode));
    EXPECT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), milestone_level));
    EXPECT_THAT(mode, Eq(flashback::practice_mode::aggressive)) << "Practice mode should remain aggressive when only one of two cards is practiced";
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), second_card.id(), duration.count(), mode));
    EXPECT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), milestone_level));
    EXPECT_THAT(mode, Eq(flashback::practice_mode::progressive)) << "Practice mode should be progressive after two cards are practiced";
}

TEST_F(test_database, get_practice_mode_from_high_levels)
{
    auto constexpr roadmap_name{"C++ Software Engineer"};
    auto constexpr subject_name{"C++"};
    auto constexpr topic_name{"Reflection"};
    auto constexpr first_headline{"The first question?"};
    auto constexpr second_headline{"The second question?"};
    auto constexpr third_headline{"The third question?"};
    auto milestone_level{flashback::expertise_level::depth};
    auto higher_level{flashback::expertise_level::origin};
    auto mode{flashback::practice_mode::aggressive};
    std::chrono::seconds duration{10};
    flashback::Roadmap roadmap{};
    flashback::Subject subject{};
    flashback::Milestone milestone{};
    flashback::Topic first_topic{};
    flashback::Topic second_topic{};
    flashback::Topic third_topic{};
    flashback::Card first_card{};
    flashback::Card second_card{};
    flashback::Card third_card{};
    first_card.set_state(flashback::Card::draft);
    first_card.set_headline(first_headline);
    second_card.set_state(flashback::Card::draft);
    second_card.set_headline(second_headline);
    third_card.set_state(flashback::Card::draft);
    third_card.set_headline(third_headline);

    ASSERT_NO_THROW(roadmap = m_database->create_roadmap(m_user->id(), roadmap_name));
    ASSERT_THAT(roadmap.id(), Gt(0));
    ASSERT_THAT(roadmap.name(), Eq(roadmap_name));
    ASSERT_NO_THROW(subject = m_database->create_subject(subject_name));
    ASSERT_THAT(subject.id(), Gt(0));
    ASSERT_THAT(subject.name(), Eq(subject_name));
    ASSERT_NO_THROW(milestone = m_database->add_milestone(subject.id(), milestone_level, roadmap.id()));
    ASSERT_THAT(milestone.position(), Eq(1));
    ASSERT_THAT(milestone.id(), subject.id());
    ASSERT_THAT(milestone.level(), milestone_level);
    ASSERT_NO_THROW(first_topic = m_database->create_topic(subject.id(), topic_name, flashback::expertise_level::surface, 0));
    ASSERT_THAT(first_topic.position(), Eq(1));
    ASSERT_THAT(first_topic.name(), topic_name);
    ASSERT_THAT(first_topic.level(), flashback::expertise_level::surface);
    ASSERT_NO_THROW(second_topic = m_database->create_topic(subject.id(), topic_name, flashback::expertise_level::depth, 0));
    ASSERT_THAT(second_topic.position(), Eq(1));
    ASSERT_THAT(second_topic.name(), topic_name);
    ASSERT_THAT(second_topic.level(), flashback::expertise_level::depth);
    ASSERT_NO_THROW(third_topic = m_database->create_topic(subject.id(), topic_name, flashback::expertise_level::origin, 0));
    ASSERT_THAT(third_topic.position(), Eq(1));
    ASSERT_THAT(third_topic.name(), topic_name);
    ASSERT_THAT(third_topic.level(), flashback::expertise_level::origin);
    ASSERT_NO_THROW(first_card = m_database->create_card(first_card));
    ASSERT_NO_THROW(second_card = m_database->create_card(second_card));
    ASSERT_NO_THROW(third_card = m_database->create_card(third_card));
    ASSERT_THAT(first_card.id(), Gt(0));
    ASSERT_THAT(second_card.id(), Gt(0));
    ASSERT_THAT(third_card.id(), Gt(0));
    ASSERT_THAT(first_card.headline(), Eq(first_headline));
    ASSERT_THAT(second_card.headline(), Eq(second_headline));
    ASSERT_THAT(third_card.headline(), Eq(third_headline));
    ASSERT_NO_THROW(m_database->add_card_to_topic(first_card.id(), subject.id(), first_topic.position(), first_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(second_card.id(), subject.id(), second_topic.position(), second_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(third_card.id(), subject.id(), third_topic.position(), third_topic.level()));
    EXPECT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::surface));
    EXPECT_THAT(mode, Eq(flashback::practice_mode::aggressive));
    EXPECT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::depth));
    EXPECT_THAT(mode, Eq(flashback::practice_mode::aggressive));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), first_card.id(), duration.count(), mode));
    EXPECT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::surface));
    EXPECT_THAT(mode, Eq(flashback::practice_mode::progressive));
    EXPECT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::depth));
    EXPECT_THAT(mode, Eq(flashback::practice_mode::aggressive));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), second_card.id(), duration.count(), mode));
    EXPECT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::surface));
    EXPECT_THAT(mode, Eq(flashback::practice_mode::progressive));
    EXPECT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::depth));
    EXPECT_THAT(mode, Eq(flashback::practice_mode::progressive));
    EXPECT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::origin));
    EXPECT_THAT(mode, Eq(flashback::practice_mode::aggressive));
    ASSERT_NO_THROW(throw_back_progress(m_user->id(), first_card.id(), std::chrono::days{10}));
    EXPECT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::surface));
    EXPECT_THAT(mode, Eq(flashback::practice_mode::aggressive));
    EXPECT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::depth));
    EXPECT_THAT(mode, Eq(flashback::practice_mode::aggressive)) << "Regardless of practice mode in depth, when surface is aggressive, above levels will be aggressive";
    EXPECT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::origin));
    EXPECT_THAT(mode, Eq(flashback::practice_mode::aggressive));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), first_card.id(), duration.count(), mode));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), third_card.id(), duration.count(), mode));
    EXPECT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::surface));
    EXPECT_THAT(mode, Eq(flashback::practice_mode::progressive));
    EXPECT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::depth));
    EXPECT_THAT(mode, Eq(flashback::practice_mode::progressive));
    EXPECT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::origin));
    EXPECT_THAT(mode, Eq(flashback::practice_mode::progressive));
}

TEST_F(test_database, get_practice_mode_long_after_practicing)
{
    auto constexpr roadmap_name{"C++ Software Engineer"};
    auto constexpr subject_name{"C++"};
    auto constexpr topic_name{"Reflection"};
    auto constexpr headline{"The first question?"};
    auto milestone_level{flashback::expertise_level::surface};
    auto cognitive_level{flashback::expertise_level::depth};
    auto mode{flashback::practice_mode::aggressive};
    std::chrono::seconds duration{10};
    flashback::Roadmap roadmap{};
    flashback::Subject subject{};
    flashback::Milestone milestone{};
    flashback::Topic topic{};
    flashback::Card card{};
    card.set_state(flashback::Card::draft);
    card.set_headline(headline);

    ASSERT_NO_THROW(roadmap = m_database->create_roadmap(m_user->id(), roadmap_name));
    ASSERT_THAT(roadmap.id(), Gt(0));
    ASSERT_THAT(roadmap.name(), Eq(roadmap_name));
    ASSERT_NO_THROW(subject = m_database->create_subject(subject_name));
    ASSERT_THAT(subject.id(), Gt(0));
    ASSERT_THAT(subject.name(), Eq(subject_name));
    ASSERT_NO_THROW(milestone = m_database->add_milestone(subject.id(), milestone_level, roadmap.id()));
    ASSERT_THAT(milestone.position(), Eq(1));
    ASSERT_THAT(milestone.id(), subject.id());
    ASSERT_THAT(milestone.level(), milestone_level);
    ASSERT_NO_THROW(topic = m_database->create_topic(subject.id(), topic_name, milestone_level, 0));
    ASSERT_THAT(topic.position(), Eq(1));
    ASSERT_THAT(topic.name(), topic_name);
    ASSERT_THAT(topic.level(), milestone_level);
    ASSERT_NO_THROW(card = m_database->create_card(card));
    ASSERT_THAT(card.id(), Gt(0));
    ASSERT_THAT(card.headline(), Eq(headline));
    ASSERT_NO_THROW(m_database->add_card_to_topic(card.id(), subject.id(), topic.position(), topic.level()));
    EXPECT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), milestone_level));
    EXPECT_THAT(mode, Eq(flashback::practice_mode::aggressive)) << "There is no progress yet, practice mode should be aggressive";
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), card.id(), duration.count(), mode));
    EXPECT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), milestone_level));
    EXPECT_THAT(mode, Eq(flashback::practice_mode::progressive)) << "Practice mode should be progressive after the only card is practiced";
    EXPECT_NO_THROW(throw_back_progress(m_user->id(), card.id(), std::chrono::days{100}));
    EXPECT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), milestone_level));
    EXPECT_THAT(mode, Eq(flashback::practice_mode::aggressive)) << "Practice mode should switch to aggressive after a long time of inactivity";
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), card.id(), duration.count(), mode));
    EXPECT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), milestone_level));
    EXPECT_THAT(mode, Eq(flashback::practice_mode::progressive));
}

TEST_F(test_database, get_practice_mode_after_new_card_available)
{
    auto constexpr roadmap_name{"C++ Software Engineer"};
    auto constexpr subject_name{"C++"};
    auto constexpr topic_name{"Reflection"};
    auto constexpr first_headline{"The first question?"};
    auto constexpr second_headline{"The second question?"};
    auto constexpr third_headline{"The third question?"};
    auto milestone_level{flashback::expertise_level::surface};
    auto cognitive_level{flashback::expertise_level::depth};
    auto mode{flashback::practice_mode::aggressive};
    std::chrono::seconds duration{10};
    flashback::Roadmap roadmap{};
    flashback::Subject subject{};
    flashback::Milestone milestone{};
    flashback::Topic topic{};
    flashback::Card first_card{};
    flashback::Card second_card{};
    flashback::Card third_card{};
    first_card.set_state(flashback::Card::draft);
    first_card.set_headline(first_headline);
    second_card.set_state(flashback::Card::draft);
    second_card.set_headline(second_headline);
    third_card.set_state(flashback::Card::draft);
    third_card.set_headline(third_headline);

    ASSERT_NO_THROW(roadmap = m_database->create_roadmap(m_user->id(), roadmap_name));
    ASSERT_THAT(roadmap.id(), Gt(0));
    ASSERT_THAT(roadmap.name(), Eq(roadmap_name));
    ASSERT_NO_THROW(subject = m_database->create_subject(subject_name));
    ASSERT_THAT(subject.id(), Gt(0));
    ASSERT_THAT(subject.name(), Eq(subject_name));
    ASSERT_NO_THROW(milestone = m_database->add_milestone(subject.id(), milestone_level, roadmap.id()));
    ASSERT_THAT(milestone.position(), Eq(1));
    ASSERT_THAT(milestone.id(), subject.id());
    ASSERT_THAT(milestone.level(), milestone_level);
    ASSERT_NO_THROW(topic = m_database->create_topic(subject.id(), topic_name, milestone_level, 0));
    ASSERT_THAT(topic.position(), Eq(1));
    ASSERT_THAT(topic.name(), topic_name);
    ASSERT_THAT(topic.level(), milestone_level);
    ASSERT_NO_THROW(first_card = m_database->create_card(first_card));
    ASSERT_NO_THROW(second_card = m_database->create_card(second_card));
    ASSERT_NO_THROW(third_card = m_database->create_card(third_card));
    ASSERT_THAT(first_card.id(), Gt(0));
    ASSERT_THAT(second_card.id(), Gt(0));
    ASSERT_THAT(third_card.id(), Gt(0));
    ASSERT_THAT(first_card.headline(), Eq(first_headline));
    ASSERT_THAT(second_card.headline(), Eq(second_headline));
    ASSERT_THAT(third_card.headline(), Eq(third_headline));
    ASSERT_NO_THROW(m_database->add_card_to_topic(first_card.id(), subject.id(), topic.position(), topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(second_card.id(), subject.id(), topic.position(), topic.level()));
    EXPECT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), milestone_level));
    EXPECT_THAT(mode, Eq(flashback::practice_mode::aggressive)) << "There is no progress yet, practice mode should be aggressive";
    EXPECT_NO_THROW(m_database->make_progress(m_user->id(), first_card.id(), duration.count(), mode));
    EXPECT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), milestone_level));
    EXPECT_THAT(mode, Eq(flashback::practice_mode::aggressive)) << "Practice mode should not switch to progressive after practicing one of the two cards";
    EXPECT_NO_THROW(m_database->make_progress(m_user->id(), second_card.id(), duration.count(), mode));
    EXPECT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), milestone_level));
    EXPECT_THAT(mode, Eq(flashback::practice_mode::progressive)) << "Practice mode should switch to progressive after practicing two cards";
    ASSERT_NO_THROW(m_database->add_card_to_topic(third_card.id(), subject.id(), topic.position(), topic.level()));
    EXPECT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), milestone_level));
    EXPECT_THAT(mode, Eq(flashback::practice_mode::aggressive)) << "Practice mode should switch to aggressive when there is a new card available";
    EXPECT_NO_THROW(m_database->make_progress(m_user->id(), third_card.id(), duration.count(), mode));
    EXPECT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), milestone_level));
    EXPECT_THAT(mode, Eq(flashback::practice_mode::progressive)) << "Practice mode should switch back to progressive when new card is practiced";
}

TEST_F(test_database, get_practice_mode_when_progress_takes_too_long)
{
    auto constexpr roadmap_name{"C++ Software Engineer"};
    auto constexpr subject_name{"C++"};
    auto constexpr topic_name{"Reflection"};
    auto constexpr first_headline{"The first question?"};
    auto constexpr second_headline{"The second question?"};
    auto constexpr third_headline{"The third question?"};
    auto milestone_level{flashback::expertise_level::surface};
    auto cognitive_level{flashback::expertise_level::depth};
    auto mode{flashback::practice_mode::aggressive};
    std::chrono::seconds duration{10};
    flashback::Roadmap roadmap{};
    flashback::Subject subject{};
    flashback::Milestone milestone{};
    flashback::Topic topic{};
    flashback::Card first_card{};
    flashback::Card second_card{};
    flashback::Card third_card{};
    first_card.set_state(flashback::Card::draft);
    first_card.set_headline(first_headline);
    second_card.set_state(flashback::Card::draft);
    second_card.set_headline(second_headline);
    third_card.set_state(flashback::Card::draft);
    third_card.set_headline(third_headline);

    ASSERT_NO_THROW(roadmap = m_database->create_roadmap(m_user->id(), roadmap_name));
    ASSERT_THAT(roadmap.id(), Gt(0));
    ASSERT_THAT(roadmap.name(), Eq(roadmap_name));
    ASSERT_NO_THROW(subject = m_database->create_subject(subject_name));
    ASSERT_THAT(subject.id(), Gt(0));
    ASSERT_THAT(subject.name(), Eq(subject_name));
    ASSERT_NO_THROW(milestone = m_database->add_milestone(subject.id(), milestone_level, roadmap.id()));
    ASSERT_THAT(milestone.position(), Eq(1));
    ASSERT_THAT(milestone.id(), subject.id());
    ASSERT_THAT(milestone.level(), milestone_level);
    ASSERT_NO_THROW(topic = m_database->create_topic(subject.id(), topic_name, milestone_level, 0));
    ASSERT_THAT(topic.position(), Eq(1));
    ASSERT_THAT(topic.name(), topic_name);
    ASSERT_THAT(topic.level(), milestone_level);
    ASSERT_NO_THROW(first_card = m_database->create_card(first_card));
    ASSERT_NO_THROW(second_card = m_database->create_card(second_card));
    ASSERT_NO_THROW(third_card = m_database->create_card(third_card));
    ASSERT_THAT(first_card.id(), Gt(0));
    ASSERT_THAT(second_card.id(), Gt(0));
    ASSERT_THAT(third_card.id(), Gt(0));
    ASSERT_THAT(first_card.headline(), Eq(first_headline));
    ASSERT_THAT(second_card.headline(), Eq(second_headline));
    ASSERT_THAT(third_card.headline(), Eq(third_headline));
    ASSERT_NO_THROW(m_database->add_card_to_topic(first_card.id(), subject.id(), topic.position(), topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(second_card.id(), subject.id(), topic.position(), topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(third_card.id(), subject.id(), topic.position(), topic.level()));
    EXPECT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), milestone_level));
    EXPECT_THAT(mode, Eq(flashback::practice_mode::aggressive));
    EXPECT_NO_THROW(m_database->make_progress(m_user->id(), first_card.id(), duration.count(), mode));
    EXPECT_NO_THROW(m_database->make_progress(m_user->id(), second_card.id(), duration.count(), mode));
    EXPECT_NO_THROW(m_database->make_progress(m_user->id(), third_card.id(), duration.count(), mode));
    EXPECT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), milestone_level));
    EXPECT_THAT(mode, Eq(flashback::practice_mode::progressive));
    EXPECT_NO_THROW(throw_back_progress(m_user->id(), first_card.id(), std::chrono::days{2}));
    EXPECT_NO_THROW(throw_back_progress(m_user->id(), second_card.id(), std::chrono::days{5}));
    EXPECT_NO_THROW(throw_back_progress(m_user->id(), third_card.id(), std::chrono::days{7}));
    EXPECT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), milestone_level));
    EXPECT_THAT(mode, Eq(flashback::practice_mode::aggressive)) << "Practice mode should switch to aggressive when the oldest practice exceeds 7 days in progressive";
    EXPECT_NO_THROW(m_database->make_progress(m_user->id(), third_card.id(), duration.count(), mode));
    EXPECT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), milestone_level));
    EXPECT_THAT(mode, Eq(flashback::practice_mode::progressive)) << "Practice mode should be back to progressive after practicing the overdue card";
}

TEST_F(test_database, get_user_cognitive_level)
{
    auto constexpr roadmap_name{"C++ Software Engineer"};
    auto constexpr subject_name{"C++"};
    auto constexpr topic_name{"Reflection"};
    auto constexpr first_headline{"The first question?"};
    auto constexpr second_headline{"The second question?"};
    auto constexpr third_headline{"The third question?"};
    auto cognitive_level{flashback::expertise_level::origin};
    auto mode{flashback::practice_mode::aggressive};
    std::chrono::seconds duration{10};
    flashback::Roadmap roadmap{};
    flashback::Subject subject{};
    flashback::Milestone milestone{};
    flashback::Topic first_topic{};
    flashback::Topic second_topic{};
    flashback::Topic third_topic{};
    flashback::Card first_card{};
    flashback::Card second_card{};
    flashback::Card third_card{};
    first_card.set_state(flashback::Card::draft);
    first_card.set_headline(first_headline);
    second_card.set_state(flashback::Card::draft);
    second_card.set_headline(second_headline);
    third_card.set_state(flashback::Card::draft);
    third_card.set_headline(third_headline);

    ASSERT_NO_THROW(roadmap = m_database->create_roadmap(m_user->id(), roadmap_name));
    ASSERT_THAT(roadmap.id(), Gt(0));
    ASSERT_THAT(roadmap.name(), Eq(roadmap_name));
    ASSERT_NO_THROW(subject = m_database->create_subject(subject_name));
    ASSERT_THAT(subject.id(), Gt(0));
    ASSERT_THAT(subject.name(), Eq(subject_name));
    ASSERT_NO_THROW(milestone = m_database->add_milestone(subject.id(), flashback::expertise_level::depth, roadmap.id()));
    ASSERT_THAT(milestone.position(), Eq(1));
    ASSERT_THAT(milestone.id(), subject.id());
    ASSERT_THAT(milestone.level(), flashback::expertise_level::depth);
    ASSERT_NO_THROW(first_topic = m_database->create_topic(subject.id(), topic_name, flashback::expertise_level::surface, 0));
    ASSERT_THAT(first_topic.position(), Eq(1));
    ASSERT_THAT(first_topic.name(), topic_name);
    ASSERT_THAT(first_topic.level(), flashback::expertise_level::surface);
    ASSERT_NO_THROW(second_topic = m_database->create_topic(subject.id(), topic_name, flashback::expertise_level::depth, 0));
    ASSERT_THAT(second_topic.position(), Eq(1));
    ASSERT_THAT(second_topic.name(), topic_name);
    ASSERT_THAT(second_topic.level(), flashback::expertise_level::depth);
    ASSERT_NO_THROW(third_topic = m_database->create_topic(subject.id(), topic_name, flashback::expertise_level::origin, 0));
    ASSERT_THAT(third_topic.position(), Eq(1));
    ASSERT_THAT(third_topic.name(), topic_name);
    ASSERT_THAT(third_topic.level(), flashback::expertise_level::origin);
    ASSERT_NO_THROW(first_card = m_database->create_card(first_card));
    ASSERT_NO_THROW(second_card = m_database->create_card(second_card));
    ASSERT_NO_THROW(third_card = m_database->create_card(third_card));
    ASSERT_THAT(first_card.id(), Gt(0));
    ASSERT_THAT(second_card.id(), Gt(0));
    ASSERT_THAT(third_card.id(), Gt(0));
    ASSERT_THAT(first_card.headline(), Eq(first_headline));
    ASSERT_THAT(second_card.headline(), Eq(second_headline));
    ASSERT_THAT(third_card.headline(), Eq(third_headline));
    ASSERT_NO_THROW(m_database->add_card_to_topic(first_card.id(), subject.id(), first_topic.position(), first_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(second_card.id(), subject.id(), second_topic.position(), second_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(third_card.id(), subject.id(), third_topic.position(), third_topic.level()));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::depth));
    EXPECT_THAT(mode, Eq(flashback::practice_mode::aggressive)) << "No progress should result in aggressive mode";
    EXPECT_NO_THROW(cognitive_level = m_database->get_user_cognitive_level(m_user->id(), roadmap.id(), subject.id()));
    EXPECT_THAT(cognitive_level, Eq(flashback::expertise_level::surface)) << "Cognitive level should be surface when none of the existing levels was practice";
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), first_card.id(), duration.count(), mode));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::surface));
    EXPECT_THAT(mode, Eq(flashback::practice_mode::progressive));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::depth));
    EXPECT_THAT(mode, Eq(flashback::practice_mode::aggressive));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::origin));
    EXPECT_THAT(mode, Eq(flashback::practice_mode::aggressive));
    EXPECT_NO_THROW(cognitive_level = m_database->get_user_cognitive_level(m_user->id(), roadmap.id(), subject.id()));
    EXPECT_THAT(cognitive_level, Eq(flashback::expertise_level::depth)) << "Cognitive level should be depth when surface is practiced";
    ASSERT_NO_THROW(throw_back_progress(m_user->id(), first_card.id(), std::chrono::days{10}));
    EXPECT_NO_THROW(cognitive_level = m_database->get_user_cognitive_level(m_user->id(), roadmap.id(), subject.id()));
    EXPECT_THAT(cognitive_level, Eq(flashback::expertise_level::surface)) << "Cognitive level should go back to surface when it is not practiced for a long time";
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), second_card.id(), duration.count(), mode));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::surface));
    EXPECT_THAT(mode, Eq(flashback::practice_mode::aggressive));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::depth));
    EXPECT_THAT(mode, Eq(flashback::practice_mode::aggressive));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::origin));
    EXPECT_THAT(mode, Eq(flashback::practice_mode::aggressive));
    EXPECT_NO_THROW(cognitive_level = m_database->get_user_cognitive_level(m_user->id(), roadmap.id(), subject.id()));
    EXPECT_THAT(cognitive_level, Eq(flashback::expertise_level::surface)) << "Cognitive level should get stuck to surface when not practice even if depth is";
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), first_card.id(), duration.count(), mode));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::surface));
    EXPECT_THAT(mode, Eq(flashback::practice_mode::progressive));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::depth));
    EXPECT_THAT(mode, Eq(flashback::practice_mode::progressive));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::origin));
    EXPECT_THAT(mode, Eq(flashback::practice_mode::aggressive));
    EXPECT_NO_THROW(cognitive_level = m_database->get_user_cognitive_level(m_user->id(), roadmap.id(), subject.id()));
    EXPECT_THAT(cognitive_level, Eq(flashback::expertise_level::depth)) << "Cognitive level should not raise to origin because the milestone is limited to depth level";
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), third_card.id(), duration.count(), mode));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::surface));
    EXPECT_THAT(mode, Eq(flashback::practice_mode::progressive));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::depth));
    EXPECT_THAT(mode, Eq(flashback::practice_mode::progressive));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::origin));
    EXPECT_THAT(mode, Eq(flashback::practice_mode::progressive));
    EXPECT_NO_THROW(cognitive_level = m_database->get_user_cognitive_level(m_user->id(), roadmap.id(), subject.id()));
    EXPECT_THAT(cognitive_level, Eq(flashback::expertise_level::depth)) << "Cognitive level should remain to depth level even when the origin level is practiced";
}

TEST_F(test_database, get_practice_topics)
{
    auto constexpr roadmap_name{"C++ Software Engineer"};
    auto constexpr subject_name{"C++"};
    auto constexpr topic_name{"Reflection"};
    auto constexpr first_headline{"The first question?"};
    auto constexpr second_headline{"The second question?"};
    auto constexpr third_headline{"The third question?"};
    auto cognitive_level{flashback::expertise_level::origin};
    auto mode{flashback::practice_mode::aggressive};
    std::chrono::seconds duration{10};
    std::vector<flashback::Topic> topics{};
    flashback::Roadmap roadmap{};
    flashback::Subject subject{};
    flashback::Milestone milestone{};
    flashback::Topic first_topic{};
    flashback::Topic second_topic{};
    flashback::Topic third_topic{};
    flashback::Card first_card{};
    flashback::Card second_card{};
    flashback::Card third_card{};
    first_card.set_state(flashback::Card::draft);
    first_card.set_headline(first_headline);
    second_card.set_state(flashback::Card::draft);
    second_card.set_headline(second_headline);
    third_card.set_state(flashback::Card::draft);
    third_card.set_headline(third_headline);

    ASSERT_NO_THROW(roadmap = m_database->create_roadmap(m_user->id(), roadmap_name));
    ASSERT_THAT(roadmap.id(), Gt(0));
    ASSERT_THAT(roadmap.name(), Eq(roadmap_name));
    ASSERT_NO_THROW(subject = m_database->create_subject(subject_name));
    ASSERT_THAT(subject.id(), Gt(0));
    ASSERT_THAT(subject.name(), Eq(subject_name));
    ASSERT_NO_THROW(milestone = m_database->add_milestone(subject.id(), flashback::expertise_level::depth, roadmap.id()));
    ASSERT_THAT(milestone.position(), Eq(1));
    ASSERT_THAT(milestone.id(), subject.id());
    ASSERT_THAT(milestone.level(), flashback::expertise_level::depth);
    ASSERT_NO_THROW(first_topic = m_database->create_topic(subject.id(), topic_name, flashback::expertise_level::surface, 0));
    ASSERT_THAT(first_topic.position(), Eq(1));
    ASSERT_THAT(first_topic.name(), topic_name);
    ASSERT_THAT(first_topic.level(), flashback::expertise_level::surface);
    ASSERT_NO_THROW(second_topic = m_database->create_topic(subject.id(), topic_name, flashback::expertise_level::depth, 0));
    ASSERT_THAT(second_topic.position(), Eq(1));
    ASSERT_THAT(second_topic.name(), topic_name);
    ASSERT_THAT(second_topic.level(), flashback::expertise_level::depth);
    ASSERT_NO_THROW(third_topic = m_database->create_topic(subject.id(), topic_name, flashback::expertise_level::origin, 0));
    ASSERT_THAT(third_topic.position(), Eq(1));
    ASSERT_THAT(third_topic.name(), topic_name);
    ASSERT_THAT(third_topic.level(), flashback::expertise_level::origin);
    ASSERT_NO_THROW(first_card = m_database->create_card(first_card));
    ASSERT_NO_THROW(second_card = m_database->create_card(second_card));
    ASSERT_NO_THROW(third_card = m_database->create_card(third_card));
    ASSERT_THAT(first_card.id(), Gt(0));
    ASSERT_THAT(second_card.id(), Gt(0));
    ASSERT_THAT(third_card.id(), Gt(0));
    ASSERT_THAT(first_card.headline(), Eq(first_headline));
    ASSERT_THAT(second_card.headline(), Eq(second_headline));
    ASSERT_THAT(third_card.headline(), Eq(third_headline));
    ASSERT_NO_THROW(m_database->add_card_to_topic(first_card.id(), subject.id(), first_topic.position(), first_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(second_card.id(), subject.id(), second_topic.position(), second_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(third_card.id(), subject.id(), third_topic.position(), third_topic.level()));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::surface));
    ASSERT_THAT(mode, Eq(flashback::practice_mode::aggressive));
    ASSERT_NO_THROW(cognitive_level = m_database->get_user_cognitive_level(m_user->id(), roadmap.id(), subject.id()));
    ASSERT_THAT(cognitive_level, Eq(flashback::expertise_level::surface));
    EXPECT_NO_THROW(topics = m_database->get_practice_topics(m_user->id(), roadmap.id(), subject.id()));
    EXPECT_THAT(topics, SizeIs(1));
    ASSERT_NO_THROW(topics.at(0).position());
    EXPECT_THAT(topics.at(0).position(), Eq(1));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), first_card.id(), duration.count(), mode));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::surface));
    ASSERT_THAT(mode, Eq(flashback::practice_mode::progressive));
    ASSERT_NO_THROW(cognitive_level = m_database->get_user_cognitive_level(m_user->id(), roadmap.id(), subject.id()));
    ASSERT_THAT(cognitive_level, Eq(flashback::expertise_level::depth));
    EXPECT_NO_THROW(topics = m_database->get_practice_topics(m_user->id(), roadmap.id(), subject.id()));
    EXPECT_THAT(topics, SizeIs(2));
    ASSERT_NO_THROW(topics.at(0).position());
    ASSERT_NO_THROW(topics.at(1).position());
    EXPECT_THAT(topics.at(0).position(), Eq(1)) << "Topic in surface level at position 1";
    EXPECT_THAT(topics.at(1).position(), Eq(1)) << "Topic in depth level at position 1";
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), second_card.id(), duration.count(), mode));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::surface));
    ASSERT_THAT(mode, Eq(flashback::practice_mode::progressive));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::depth));
    ASSERT_THAT(mode, Eq(flashback::practice_mode::progressive));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::origin));
    ASSERT_THAT(mode, Eq(flashback::practice_mode::aggressive));
    ASSERT_NO_THROW(cognitive_level = m_database->get_user_cognitive_level(m_user->id(), roadmap.id(), subject.id()));
    ASSERT_THAT(cognitive_level, Eq(flashback::expertise_level::depth)) << "Cognitive level should remain in depth since milestone is in depth level";
    EXPECT_NO_THROW(topics = m_database->get_practice_topics(m_user->id(), roadmap.id(), subject.id()));
    EXPECT_THAT(topics, SizeIs(2));
    ASSERT_NO_THROW(topics.at(0).position());
    ASSERT_NO_THROW(topics.at(1).position());
    EXPECT_THAT(topics.at(0).position(), Eq(1)) << "Topic in surface level at position 1";
    EXPECT_THAT(topics.at(1).position(), Eq(1)) << "Topic in depth level at position 1";
}

TEST_F(test_database, get_practice_cards)
{
    auto constexpr roadmap_name{"C++ Software Engineer"};
    auto constexpr subject_name{"C++"};
    auto constexpr topic_name{"Reflection"};
    auto constexpr first_headline{"The first question?"};
    auto constexpr second_headline{"The second question?"};
    auto constexpr third_headline{"The third question?"};
    auto cognitive_level{flashback::expertise_level::origin};
    auto mode{flashback::practice_mode::aggressive};
    std::chrono::seconds duration{10};
    std::vector<flashback::Topic> topics{};
    std::vector<flashback::Card> cards{};
    flashback::Roadmap roadmap{};
    flashback::Subject subject{};
    flashback::Milestone milestone{};
    flashback::Topic first_topic{};
    flashback::Topic second_topic{};
    flashback::Topic third_topic{};
    flashback::Card first_card{};
    flashback::Card second_card{};
    flashback::Card third_card{};
    first_card.set_state(flashback::Card::draft);
    first_card.set_headline(first_headline);
    second_card.set_state(flashback::Card::draft);
    second_card.set_headline(second_headline);
    third_card.set_state(flashback::Card::draft);
    third_card.set_headline(third_headline);

    ASSERT_NO_THROW(roadmap = m_database->create_roadmap(m_user->id(), roadmap_name));
    ASSERT_THAT(roadmap.id(), Gt(0));
    ASSERT_THAT(roadmap.name(), Eq(roadmap_name));
    ASSERT_NO_THROW(subject = m_database->create_subject(subject_name));
    ASSERT_THAT(subject.id(), Gt(0));
    ASSERT_THAT(subject.name(), Eq(subject_name));
    ASSERT_NO_THROW(milestone = m_database->add_milestone(subject.id(), flashback::expertise_level::depth, roadmap.id()));
    ASSERT_THAT(milestone.position(), Eq(1));
    ASSERT_THAT(milestone.id(), subject.id());
    ASSERT_THAT(milestone.level(), flashback::expertise_level::depth);
    ASSERT_NO_THROW(first_topic = m_database->create_topic(subject.id(), topic_name, flashback::expertise_level::surface, 0));
    ASSERT_THAT(first_topic.position(), Eq(1));
    ASSERT_THAT(first_topic.name(), topic_name);
    ASSERT_THAT(first_topic.level(), flashback::expertise_level::surface);
    ASSERT_NO_THROW(second_topic = m_database->create_topic(subject.id(), topic_name, flashback::expertise_level::depth, 0));
    ASSERT_THAT(second_topic.position(), Eq(1));
    ASSERT_THAT(second_topic.name(), topic_name);
    ASSERT_THAT(second_topic.level(), flashback::expertise_level::depth);
    ASSERT_NO_THROW(third_topic = m_database->create_topic(subject.id(), topic_name, flashback::expertise_level::origin, 0));
    ASSERT_THAT(third_topic.position(), Eq(1));
    ASSERT_THAT(third_topic.name(), topic_name);
    ASSERT_THAT(third_topic.level(), flashback::expertise_level::origin);
    ASSERT_NO_THROW(first_card = m_database->create_card(first_card));
    ASSERT_NO_THROW(second_card = m_database->create_card(second_card));
    ASSERT_NO_THROW(third_card = m_database->create_card(third_card));
    ASSERT_THAT(first_card.id(), Gt(0));
    ASSERT_THAT(second_card.id(), Gt(0));
    ASSERT_THAT(third_card.id(), Gt(0));
    ASSERT_THAT(first_card.headline(), Eq(first_headline));
    ASSERT_THAT(second_card.headline(), Eq(second_headline));
    ASSERT_THAT(third_card.headline(), Eq(third_headline));
    ASSERT_NO_THROW(m_database->add_card_to_topic(first_card.id(), subject.id(), first_topic.position(), first_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(second_card.id(), subject.id(), second_topic.position(), second_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(third_card.id(), subject.id(), third_topic.position(), third_topic.level()));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::surface));
    ASSERT_THAT(mode, Eq(flashback::practice_mode::aggressive));
    ASSERT_NO_THROW(cognitive_level = m_database->get_user_cognitive_level(m_user->id(), roadmap.id(), subject.id()));
    ASSERT_THAT(cognitive_level, Eq(flashback::expertise_level::surface));
    EXPECT_NO_THROW(topics = m_database->get_practice_topics(m_user->id(), roadmap.id(), subject.id()));
    EXPECT_THAT(topics, SizeIs(1)) << "Only the first topic should be accessible to the user when there were no practices";
    ASSERT_NO_THROW(topics.at(0).position());
    EXPECT_THAT(topics.at(0).position(), Eq(1));
    EXPECT_NO_THROW(cards = m_database->get_practice_cards(m_user->id(), roadmap.id(), subject.id(), first_topic.level(), first_topic.position()));
    EXPECT_THAT(cards, SizeIs(1)) << "The cards from first topic should be available to practice";
    ASSERT_NO_THROW(cards.at(0).id());
    EXPECT_THAT(cards.at(0).id(), first_card.id());
    EXPECT_NO_THROW(cards = m_database->get_practice_cards(m_user->id(), roadmap.id(), subject.id(), second_topic.level(), second_topic.position()));
    EXPECT_THAT(cards, IsEmpty()) << "Cards from other levels should not be accessible when the user has not reached to the end of practice in previous levels";
    EXPECT_NO_THROW(cards = m_database->get_practice_cards(m_user->id(), roadmap.id(), subject.id(), third_topic.level(), third_topic.position()));
    EXPECT_THAT(cards, IsEmpty()) << "Cards from other levels should not be accessible when the user has not reached to the end of practice in previous levels";
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), first_card.id(), duration.count(), mode));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::surface));
    ASSERT_THAT(mode, Eq(flashback::practice_mode::progressive));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::depth));
    ASSERT_THAT(mode, Eq(flashback::practice_mode::aggressive));
    ASSERT_NO_THROW(cognitive_level = m_database->get_user_cognitive_level(m_user->id(), roadmap.id(), subject.id()));
    ASSERT_THAT(cognitive_level, Eq(flashback::expertise_level::depth));
    EXPECT_NO_THROW(topics = m_database->get_practice_topics(m_user->id(), roadmap.id(), subject.id()));
    EXPECT_THAT(topics, SizeIs(2)) << "Two levels of expertise should be accessible since the user finished practicing the first level";
    ASSERT_NO_THROW(topics.at(0).position());
    EXPECT_THAT(topics.at(0).position(), Eq(1));
    ASSERT_NO_THROW(topics.at(1).position());
    EXPECT_THAT(topics.at(1).position(), Eq(1));
    EXPECT_NO_THROW(cards = m_database->get_practice_cards(m_user->id(), roadmap.id(), subject.id(), first_topic.level(), first_topic.position()));
    EXPECT_THAT(cards, IsEmpty()) << "Cards from the first level should disappear since user recently finished practicing";
    EXPECT_NO_THROW(cards = m_database->get_practice_cards(m_user->id(), roadmap.id(), subject.id(), second_topic.level(), second_topic.position()));
    EXPECT_THAT(cards, SizeIs(1)) << "Only the cards from the levels which user has not finished practicing yet should appear";
    ASSERT_NO_THROW(cards.at(0).id());
    EXPECT_THAT(cards.at(0).id(), second_card.id());
    EXPECT_NO_THROW(cards = m_database->get_practice_cards(m_user->id(), roadmap.id(), subject.id(), third_topic.level(), third_topic.position()));
    EXPECT_THAT(cards, IsEmpty()) << "Cards from levels above the level of milestone should never appear";
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), second_card.id(), duration.count(), mode));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::surface));
    ASSERT_THAT(mode, Eq(flashback::practice_mode::progressive));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::depth));
    ASSERT_THAT(mode, Eq(flashback::practice_mode::progressive));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::origin));
    ASSERT_THAT(mode, Eq(flashback::practice_mode::aggressive));
    ASSERT_NO_THROW(cognitive_level = m_database->get_user_cognitive_level(m_user->id(), roadmap.id(), subject.id()));
    ASSERT_THAT(cognitive_level, Eq(flashback::expertise_level::depth));
    EXPECT_NO_THROW(topics = m_database->get_practice_topics(m_user->id(), roadmap.id(), subject.id()));
    EXPECT_THAT(topics, SizeIs(2));
    ASSERT_NO_THROW(topics.at(0).position());
    EXPECT_THAT(topics.at(0).position(), Eq(1));
    ASSERT_NO_THROW(topics.at(1).position());
    EXPECT_THAT(topics.at(1).position(), Eq(1));
    EXPECT_NO_THROW(cards = m_database->get_practice_cards(m_user->id(), roadmap.id(), subject.id(), first_topic.level(), first_topic.position()));
    EXPECT_THAT(cards, SizeIs(1)) << "When user finished practicing all accessible levels, all cards should appear as progressive practicing";
    EXPECT_NO_THROW(cards = m_database->get_practice_cards(m_user->id(), roadmap.id(), subject.id(), second_topic.level(), second_topic.position()));
    EXPECT_THAT(cards, SizeIs(1)) << "When user finished practicing all accessible levels, all cards should appear as progressive practicing";
    EXPECT_NO_THROW(cards = m_database->get_practice_cards(m_user->id(), roadmap.id(), subject.id(), third_topic.level(), third_topic.position()));
    EXPECT_THAT(cards, IsEmpty()) << "Cards from levels above the level of milestone should never appear";
}

TEST_F(test_database, mark_section_as_reviewed)
{
    flashback::Resource resource{};
    flashback::Section first_section{};
    flashback::Section second_section{};
    flashback::Section third_section{};
    resource.set_name("C++ Resource");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.set_link("https://flashback.eu.com");
    resource.set_production(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
    resource.set_expiration(std::chrono::duration_cast<std::chrono::seconds>((std::chrono::system_clock::now() + std::chrono::years(3)).time_since_epoch()).count());
    first_section.set_name("Class");
    second_section.set_name("Struct");
    third_section.set_name("Lambda");

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(first_section = m_database->create_section(resource.id(), 0, first_section.name(), first_section.link()));
    ASSERT_THAT(first_section.position(), Eq(1));
    ASSERT_NO_THROW(second_section = m_database->create_section(resource.id(), 0, second_section.name(), second_section.link()));
    ASSERT_THAT(second_section.position(), Eq(2));
    ASSERT_NO_THROW(third_section = m_database->create_section(resource.id(), 0, third_section.name(), third_section.link()));
    ASSERT_THAT(third_section.position(), Eq(3));
    EXPECT_NO_THROW(m_database->mark_section_as_reviewed(resource.id(), first_section.position()));
    EXPECT_NO_THROW(m_database->mark_section_as_reviewed(resource.id(), second_section.position()));
    EXPECT_NO_THROW(m_database->mark_section_as_reviewed(resource.id(), third_section.position()));
}

TEST_F(test_database, mark_section_as_completed)
{
    flashback::Resource resource{};
    flashback::Section first_section{};
    flashback::Section second_section{};
    flashback::Section third_section{};
    resource.set_name("C++ Resource");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.set_link("https://flashback.eu.com");
    resource.set_production(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
    resource.set_expiration(std::chrono::duration_cast<std::chrono::seconds>((std::chrono::system_clock::now() + std::chrono::years(3)).time_since_epoch()).count());
    first_section.set_name("Class");
    second_section.set_name("Struct");
    third_section.set_name("Lambda");

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(first_section = m_database->create_section(resource.id(), 0, first_section.name(), first_section.link()));
    ASSERT_THAT(first_section.position(), Eq(1));
    ASSERT_NO_THROW(second_section = m_database->create_section(resource.id(), 0, second_section.name(), second_section.link()));
    ASSERT_THAT(second_section.position(), Eq(2));
    ASSERT_NO_THROW(third_section = m_database->create_section(resource.id(), 0, third_section.name(), third_section.link()));
    ASSERT_THAT(third_section.position(), Eq(3));
    EXPECT_NO_THROW(m_database->mark_section_as_completed(resource.id(), first_section.position()));
    EXPECT_NO_THROW(m_database->mark_section_as_completed(resource.id(), second_section.position()));
    EXPECT_NO_THROW(m_database->mark_section_as_completed(resource.id(), third_section.position()));
}

TEST_F(test_database, get_resource_state)
{
    flashback::closure_state state{};
    flashback::Resource resource{};
    flashback::Section first_section{};
    flashback::Section second_section{};
    flashback::Section third_section{};
    resource.set_name("C++ Resource");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.set_link("https://flashback.eu.com");
    resource.set_production(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
    resource.set_expiration(std::chrono::duration_cast<std::chrono::seconds>((std::chrono::system_clock::now() + std::chrono::years(3)).time_since_epoch()).count());
    first_section.set_name("Class");
    second_section.set_name("Struct");
    third_section.set_name("Lambda");

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    EXPECT_NO_THROW(state = m_database->get_resource_state(resource.id()));
    EXPECT_THAT(state, Eq(flashback::closure_state::draft));
    ASSERT_NO_THROW(first_section = m_database->create_section(resource.id(), 0, first_section.name(), first_section.link()));
    ASSERT_THAT(first_section.position(), Eq(1));
    EXPECT_NO_THROW(state = m_database->get_resource_state(resource.id()));
    EXPECT_THAT(state, Eq(flashback::closure_state::draft));
    EXPECT_NO_THROW(m_database->mark_section_as_reviewed(resource.id(), first_section.position()));
    EXPECT_NO_THROW(state = m_database->get_resource_state(resource.id()));
    EXPECT_THAT(state, Eq(flashback::closure_state::reviewed));
    EXPECT_NO_THROW(m_database->mark_section_as_completed(resource.id(), first_section.position()));
    EXPECT_NO_THROW(state = m_database->get_resource_state(resource.id()));
    EXPECT_THAT(state, Eq(flashback::closure_state::completed));
}

TEST_F(test_database, study)
{
    flashback::Resource resource{};
    flashback::Section section{};
    flashback::Card card{};
    resource.set_name("C++ Resource");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.set_link("https://flashback.eu.com");
    resource.set_production(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
    resource.set_expiration(std::chrono::duration_cast<std::chrono::seconds>((std::chrono::system_clock::now() + std::chrono::years(3)).time_since_epoch()).count());
    section.set_name("Class");
    card.set_headline("Is this a study?");

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(section = m_database->create_section(resource.id(), 0, section.name(), section.link()));
    ASSERT_THAT(section.position(), Eq(1));
    ASSERT_NO_THROW(card = m_database->create_card(card));
    ASSERT_THAT(card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(card.id(), resource.id(), section.position()));
    EXPECT_NO_THROW(m_database->study(m_user->id(), card.id(), std::chrono::seconds{20}));
}

TEST_F(test_database, get_study_resources)
{
    flashback::Roadmap roadmap{};
    flashback::Subject subject{};
    flashback::Milestone milestone{};
    flashback::Topic topic{};
    flashback::Resource first_resource{};
    flashback::Resource second_resource{};
    flashback::Resource third_resource{};
    flashback::Section first_section{};
    flashback::Section second_section{};
    flashback::Section third_section{};
    flashback::Card first_card{};
    flashback::Card second_card{};
    flashback::Card third_card{};
    flashback::practice_mode mode{};
    std::vector<flashback::Resource> resources{};
    std::map<uint64_t, flashback::Resource> studying_resources{};

    roadmap.set_name("C++ Software Engineer");
    subject.set_name("C++");
    milestone.set_level(flashback::expertise_level::depth);
    topic.set_name("Reflection");
    topic.set_level(flashback::expertise_level::surface);
    first_resource.set_name("First C++ Resource");
    first_resource.set_type(flashback::Resource::book);
    first_resource.set_pattern(flashback::Resource::chapter);
    second_resource.set_name("Second C++ Resource");
    second_resource.set_type(flashback::Resource::manual);
    second_resource.set_pattern(flashback::Resource::page);
    third_resource.set_name("Personal Knowledge");
    third_resource.set_type(flashback::Resource::nerve);
    third_resource.set_pattern(flashback::Resource::synapse);
    first_section.set_name("First C++ Resource Chapter 1");
    second_section.set_name("Second C++ Resource Chapter 1");
    third_section.set_name("Coroutine");
    first_card.set_headline("First Card");
    second_card.set_headline("Second Card");
    third_card.set_headline("Third Card");

    ASSERT_NO_THROW(roadmap = m_database->create_roadmap(m_user->id(), roadmap.name()));
    ASSERT_THAT(roadmap.id(), Gt(0));
    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_THAT(subject.id(), Gt(0));
    ASSERT_NO_THROW(milestone = m_database->add_milestone(subject.id(), milestone.level(), roadmap.id()));
    ASSERT_THAT(milestone.position(), Eq(1));
    ASSERT_NO_THROW(topic = m_database->create_topic(subject.id(), topic.name(), topic.level(), 0));
    ASSERT_THAT(topic.position(), Eq(1));
    ASSERT_NO_THROW(first_resource = m_database->create_resource(first_resource));
    ASSERT_THAT(first_resource.id(), Gt(0));
    ASSERT_NO_THROW(second_resource = m_database->create_resource(second_resource));
    ASSERT_THAT(second_resource.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_resource_to_subject(first_resource.id(), subject.id()));
    ASSERT_NO_THROW(m_database->add_resource_to_subject(second_resource.id(), subject.id()));
    ASSERT_NO_THROW(third_resource = m_database->create_nerve(m_user->id(), third_resource.name(), subject.id(), 0));
    ASSERT_THAT(third_resource.id(), Gt(0));
    ASSERT_NO_THROW(first_section = m_database->create_section(first_resource.id(), 0, first_section.name(), first_section.link()));
    ASSERT_THAT(first_section.position(), Eq(1));
    ASSERT_NO_THROW(second_section = m_database->create_section(second_resource.id(), 0, second_section.name(), second_section.link()));
    ASSERT_THAT(second_section.position(), Eq(1));
    ASSERT_NO_THROW(third_section = m_database->create_section(third_resource.id(), 0, third_section.name(), third_section.link()));
    ASSERT_THAT(third_section.position(), Eq(1));
    ASSERT_NO_THROW(first_card = m_database->create_card(first_card));
    ASSERT_THAT(first_card.id(), Gt(0));
    ASSERT_NO_THROW(second_card = m_database->create_card(second_card));
    ASSERT_THAT(second_card.id(), Gt(0));
    ASSERT_NO_THROW(third_card = m_database->create_card(third_card));
    ASSERT_THAT(third_card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(first_card.id(), first_resource.id(), first_section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_section(second_card.id(), second_resource.id(), second_section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_section(third_card.id(), third_resource.id(), third_section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(first_card.id(), subject.id(), topic.position(), topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(second_card.id(), subject.id(), topic.position(), topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(third_card.id(), subject.id(), topic.position(), topic.level()));
    ASSERT_NO_THROW(resources = m_database->get_resources(m_user->id(), subject.id()));
    EXPECT_THAT(resources, SizeIs(3));
    EXPECT_NO_THROW(studying_resources = m_database->get_study_resources(m_user->id()));
    EXPECT_THAT(studying_resources, IsEmpty()) << "There should be no resources when none of the resources were studied";
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), milestone.level()));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), first_card.id(), 20, mode));
    EXPECT_NO_THROW(studying_resources = m_database->get_study_resources(m_user->id()));
    EXPECT_THAT(studying_resources, IsEmpty()) << "There should be no resources when no study was recorded even when cards from existing resources are practiced";
    ASSERT_NO_THROW(m_database->study(m_user->id(), first_card.id(), std::chrono::seconds{10}));
    EXPECT_NO_THROW(studying_resources = m_database->get_study_resources(m_user->id()));
    EXPECT_THAT(studying_resources, SizeIs(1)) << "One card was studied so there should be one resource in the studied resources list";
    ASSERT_NO_THROW(studying_resources.at(1).id());
    EXPECT_THAT(studying_resources.at(1).id(), Eq(first_resource.id())) << "First resource was explicitly marked as studied and should be in the studied resources";
    ASSERT_NO_THROW(m_database->study(m_user->id(), second_card.id(), std::chrono::seconds{10}));
    EXPECT_NO_THROW(studying_resources = m_database->get_study_resources(m_user->id()));
    EXPECT_THAT(studying_resources, SizeIs(2));
    ASSERT_NO_THROW(studying_resources.at(1).id());
    EXPECT_THAT(studying_resources.at(1).id(), Eq(second_resource.id())) << "Second resource was studied more recently, so it should appear as first studied resource";
    EXPECT_THAT(studying_resources.at(2).id(), Eq(first_resource.id())) << "First resource was studied before the last, so it should appear as the second studied resource";
    ASSERT_NO_THROW(m_database->study(m_user->id(), third_card.id(), std::chrono::seconds{10}));
    EXPECT_NO_THROW(studying_resources = m_database->get_study_resources(m_user->id()));
    EXPECT_THAT(studying_resources, SizeIs(3)) << "Even if the resource is a nerve that belongs to the user it should appear as a studying resource";
    ASSERT_NO_THROW(studying_resources.at(1).id());
    EXPECT_THAT(studying_resources.at(1).id(), Eq(third_resource.id())) << "Third resource was studied more recently, so it should appear as first studied resource";
    EXPECT_THAT(studying_resources.at(2).id(), Eq(second_resource.id())) << "Second resource was studied before the last, so it should appear as the second studied resource";
    EXPECT_THAT(studying_resources.at(3).id(), Eq(first_resource.id())) << "First resource was studied first, so it should appear as the last studied resource";
}

TEST_F(test_database, mark_card_as_reviewed)
{
    flashback::Resource resource{};
    flashback::Section section{};
    flashback::Card card{};
    std::vector<flashback::Card> cards{};
    resource.set_name("C++ Resource");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.set_link("https://flashback.eu.com");
    resource.set_production(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
    resource.set_expiration(std::chrono::duration_cast<std::chrono::seconds>((std::chrono::system_clock::now() + std::chrono::years(3)).time_since_epoch()).count());
    section.set_name("Class");
    card.set_headline("Some random headline");

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(section = m_database->create_section(resource.id(), 0, section.name(), section.link()));
    ASSERT_THAT(section.position(), Eq(1));
    ASSERT_NO_THROW(card = m_database->create_card(card));
    ASSERT_THAT(card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(cards = m_database->get_section_cards(resource.id(), section.position()));
    ASSERT_THAT(cards, SizeIs(1));
    ASSERT_NO_THROW(cards.at(0).id());
    ASSERT_THAT(cards.at(0).id(), card.id());
    ASSERT_THAT(cards.at(0).state(), flashback::Card::draft);
    EXPECT_NO_THROW(m_database->mark_card_as_reviewed(card.id()));
    EXPECT_NO_THROW(cards = m_database->get_section_cards(resource.id(), section.position()));
    EXPECT_THAT(cards, SizeIs(1));
    ASSERT_NO_THROW(cards.at(0).id());
    EXPECT_THAT(cards.at(0).id(), card.id());
    EXPECT_THAT(cards.at(0).state(), flashback::Card::reviewed);
}

TEST_F(test_database, mark_card_as_completed)
{
    flashback::Resource resource{};
    flashback::Section section{};
    flashback::Card card{};
    std::vector<flashback::Card> cards{};
    resource.set_name("C++ Resource");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.set_link("https://flashback.eu.com");
    resource.set_production(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
    resource.set_expiration(std::chrono::duration_cast<std::chrono::seconds>((std::chrono::system_clock::now() + std::chrono::years(3)).time_since_epoch()).count());
    section.set_name("Class");
    card.set_headline("Some random headline");

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(section = m_database->create_section(resource.id(), 0, section.name(), section.link()));
    ASSERT_THAT(section.position(), Eq(1));
    ASSERT_NO_THROW(card = m_database->create_card(card));
    ASSERT_THAT(card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(cards = m_database->get_section_cards(resource.id(), section.position()));
    ASSERT_THAT(cards, SizeIs(1));
    ASSERT_NO_THROW(cards.at(0).id());
    ASSERT_THAT(cards.at(0).id(), card.id());
    ASSERT_THAT(cards.at(0).state(), flashback::Card::draft);
    EXPECT_NO_THROW(m_database->mark_card_as_completed(card.id()));
    EXPECT_NO_THROW(cards = m_database->get_section_cards(resource.id(), section.position()));
    EXPECT_THAT(cards, SizeIs(1));
    ASSERT_NO_THROW(cards.at(0).id());
    EXPECT_THAT(cards.at(0).id(), card.id());
    EXPECT_THAT(cards.at(0).state(), flashback::Card::draft);
    EXPECT_NO_THROW(m_database->mark_card_as_reviewed(card.id()));
    EXPECT_NO_THROW(m_database->mark_card_as_completed(card.id()));
    EXPECT_NO_THROW(cards = m_database->get_section_cards(resource.id(), section.position()));
    EXPECT_THAT(cards, SizeIs(1));
    ASSERT_NO_THROW(cards.at(0).id());
    EXPECT_THAT(cards.at(0).id(), card.id());
    EXPECT_THAT(cards.at(0).state(), flashback::Card::completed);
    EXPECT_NO_THROW(m_database->mark_card_as_reviewed(card.id()));
    EXPECT_NO_THROW(cards = m_database->get_section_cards(resource.id(), section.position()));
    EXPECT_THAT(cards, SizeIs(1));
    ASSERT_NO_THROW(cards.at(0).id());
    EXPECT_THAT(cards.at(0).id(), card.id());
    EXPECT_THAT(cards.at(0).state(), flashback::Card::reviewed);
}

TEST_F(test_database, mark_card_as_approved)
{
    flashback::Resource resource{};
    flashback::Section section{};
    flashback::Card card{};
    std::vector<flashback::Card> cards{};
    resource.set_name("C++ Resource");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.set_link("https://flashback.eu.com");
    resource.set_production(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
    resource.set_expiration(std::chrono::duration_cast<std::chrono::seconds>((std::chrono::system_clock::now() + std::chrono::years(3)).time_since_epoch()).count());
    section.set_name("Class");
    card.set_headline("Some random headline");

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(section = m_database->create_section(resource.id(), 0, section.name(), section.link()));
    ASSERT_THAT(section.position(), Eq(1));
    ASSERT_NO_THROW(card = m_database->create_card(card));
    ASSERT_THAT(card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(cards = m_database->get_section_cards(resource.id(), section.position()));
    ASSERT_THAT(cards, SizeIs(1));
    ASSERT_NO_THROW(cards.at(0).id());
    ASSERT_THAT(cards.at(0).id(), card.id());
    ASSERT_THAT(cards.at(0).state(), flashback::Card::draft);
    EXPECT_NO_THROW(m_database->mark_card_as_approved(card.id()));
    EXPECT_NO_THROW(cards = m_database->get_section_cards(resource.id(), section.position()));
    EXPECT_THAT(cards, SizeIs(1));
    ASSERT_NO_THROW(cards.at(0).id());
    EXPECT_THAT(cards.at(0).id(), card.id());
    EXPECT_THAT(cards.at(0).state(), flashback::Card::draft);
    EXPECT_NO_THROW(m_database->mark_card_as_reviewed(card.id()));
    EXPECT_NO_THROW(m_database->mark_card_as_approved(card.id()));
    EXPECT_NO_THROW(cards = m_database->get_section_cards(resource.id(), section.position()));
    EXPECT_THAT(cards, SizeIs(1));
    ASSERT_NO_THROW(cards.at(0).id());
    EXPECT_THAT(cards.at(0).id(), card.id());
    EXPECT_THAT(cards.at(0).state(), flashback::Card::reviewed);
    EXPECT_NO_THROW(m_database->mark_card_as_completed(card.id()));
    EXPECT_NO_THROW(m_database->mark_card_as_approved(card.id()));
    EXPECT_NO_THROW(cards = m_database->get_section_cards(resource.id(), section.position()));
    EXPECT_THAT(cards, SizeIs(1));
    ASSERT_NO_THROW(cards.at(0).id());
    EXPECT_THAT(cards.at(0).id(), card.id());
    EXPECT_THAT(cards.at(0).state(), flashback::Card::approved);
}

TEST_F(test_database, mark_card_as_released)
{
    flashback::Resource resource{};
    flashback::Section section{};
    flashback::Card card{};
    std::vector<flashback::Card> cards{};
    resource.set_name("C++ Resource");
    resource.set_type(flashback::Resource::book);
    resource.set_pattern(flashback::Resource::chapter);
    resource.set_link("https://flashback.eu.com");
    resource.set_production(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
    resource.set_expiration(std::chrono::duration_cast<std::chrono::seconds>((std::chrono::system_clock::now() + std::chrono::years(3)).time_since_epoch()).count());
    section.set_name("Class");
    card.set_headline("Some random headline");

    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(section = m_database->create_section(resource.id(), 0, section.name(), section.link()));
    ASSERT_THAT(section.position(), Eq(1));
    ASSERT_NO_THROW(card = m_database->create_card(card));
    ASSERT_THAT(card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(cards = m_database->get_section_cards(resource.id(), section.position()));
    ASSERT_THAT(cards, SizeIs(1));
    ASSERT_NO_THROW(cards.at(0).id());
    ASSERT_THAT(cards.at(0).id(), card.id());
    ASSERT_THAT(cards.at(0).state(), flashback::Card::draft);
    EXPECT_NO_THROW(m_database->mark_card_as_released(card.id()));
    EXPECT_NO_THROW(cards = m_database->get_section_cards(resource.id(), section.position()));
    EXPECT_THAT(cards, SizeIs(1));
    ASSERT_NO_THROW(cards.at(0).id());
    EXPECT_THAT(cards.at(0).id(), card.id());
    EXPECT_THAT(cards.at(0).state(), flashback::Card::draft);
    EXPECT_NO_THROW(m_database->mark_card_as_reviewed(card.id()));
    EXPECT_NO_THROW(m_database->mark_card_as_released(card.id()));
    EXPECT_NO_THROW(cards = m_database->get_section_cards(resource.id(), section.position()));
    EXPECT_THAT(cards, SizeIs(1));
    ASSERT_NO_THROW(cards.at(0).id());
    EXPECT_THAT(cards.at(0).id(), card.id());
    EXPECT_THAT(cards.at(0).state(), flashback::Card::reviewed);
    EXPECT_NO_THROW(m_database->mark_card_as_completed(card.id()));
    EXPECT_NO_THROW(m_database->mark_card_as_released(card.id()));
    EXPECT_NO_THROW(cards = m_database->get_section_cards(resource.id(), section.position()));
    EXPECT_THAT(cards, SizeIs(1));
    ASSERT_NO_THROW(cards.at(0).id());
    EXPECT_THAT(cards.at(0).id(), card.id());
    EXPECT_THAT(cards.at(0).state(), flashback::Card::completed);
    EXPECT_NO_THROW(m_database->mark_card_as_approved(card.id()));
    EXPECT_NO_THROW(m_database->mark_card_as_released(card.id()));
    EXPECT_NO_THROW(cards = m_database->get_section_cards(resource.id(), section.position()));
    EXPECT_THAT(cards, SizeIs(1));
    ASSERT_NO_THROW(cards.at(0).id());
    EXPECT_THAT(cards.at(0).id(), card.id());
    EXPECT_THAT(cards.at(0).state(), flashback::Card::released);
}

TEST_F(test_database, get_progress_weight)
{
    flashback::Roadmap roadmap{};
    flashback::Subject subject{};
    flashback::Milestone milestone{};
    flashback::Topic topic{};
    flashback::Resource first_resource{};
    flashback::Resource second_resource{};
    flashback::Resource third_resource{};
    flashback::Section first_section{};
    flashback::Section second_section{};
    flashback::Section third_section{};
    flashback::Card first_card{};
    flashback::Card second_card{};
    flashback::Card third_card{};
    flashback::practice_mode mode{};

    roadmap.set_name("C++ Software Engineer");
    subject.set_name("C++");
    milestone.set_level(flashback::expertise_level::depth);
    topic.set_name("Reflection");
    topic.set_level(flashback::expertise_level::surface);
    first_resource.set_name("First C++ Resource");
    first_resource.set_type(flashback::Resource::book);
    first_resource.set_pattern(flashback::Resource::chapter);
    second_resource.set_name("Second C++ Resource");
    second_resource.set_type(flashback::Resource::manual);
    second_resource.set_pattern(flashback::Resource::page);
    third_resource.set_name("Personal Knowledge");
    third_resource.set_type(flashback::Resource::nerve);
    third_resource.set_pattern(flashback::Resource::synapse);
    first_section.set_name("First C++ Resource Chapter 1");
    second_section.set_name("Second C++ Resource Chapter 1");
    third_section.set_name("Coroutine");
    first_card.set_headline("First Card");
    second_card.set_headline("Second Card");
    third_card.set_headline("Third Card");

    ASSERT_NO_THROW(roadmap = m_database->create_roadmap(m_user->id(), roadmap.name()));
    ASSERT_THAT(roadmap.id(), Gt(0));
    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_THAT(subject.id(), Gt(0));
    ASSERT_NO_THROW(milestone = m_database->add_milestone(subject.id(), milestone.level(), roadmap.id()));
    ASSERT_THAT(milestone.position(), Eq(1));
    ASSERT_NO_THROW(topic = m_database->create_topic(subject.id(), topic.name(), topic.level(), 0));
    ASSERT_THAT(topic.position(), Eq(1));
    ASSERT_NO_THROW(first_resource = m_database->create_resource(first_resource));
    ASSERT_THAT(first_resource.id(), Gt(0));
    ASSERT_NO_THROW(second_resource = m_database->create_resource(second_resource));
    ASSERT_THAT(second_resource.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_resource_to_subject(first_resource.id(), subject.id()));
    ASSERT_NO_THROW(m_database->add_resource_to_subject(second_resource.id(), subject.id()));
    ASSERT_NO_THROW(third_resource = m_database->create_nerve(m_user->id(), third_resource.name(), subject.id(), 0));
    ASSERT_THAT(third_resource.id(), Gt(0));
    ASSERT_NO_THROW(first_section = m_database->create_section(first_resource.id(), 0, first_section.name(), first_section.link()));
    ASSERT_THAT(first_section.position(), Eq(1));
    ASSERT_NO_THROW(second_section = m_database->create_section(second_resource.id(), 0, second_section.name(), second_section.link()));
    ASSERT_THAT(second_section.position(), Eq(1));
    ASSERT_NO_THROW(third_section = m_database->create_section(third_resource.id(), 0, third_section.name(), third_section.link()));
    ASSERT_THAT(third_section.position(), Eq(1));
    ASSERT_NO_THROW(first_card = m_database->create_card(first_card));
    ASSERT_THAT(first_card.id(), Gt(0));
    ASSERT_NO_THROW(second_card = m_database->create_card(second_card));
    ASSERT_THAT(second_card.id(), Gt(0));
    ASSERT_NO_THROW(third_card = m_database->create_card(third_card));
    ASSERT_THAT(third_card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(first_card.id(), first_resource.id(), first_section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_section(second_card.id(), second_resource.id(), second_section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_section(third_card.id(), third_resource.id(), third_section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(first_card.id(), subject.id(), topic.position(), topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(second_card.id(), subject.id(), topic.position(), topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(third_card.id(), subject.id(), topic.position(), topic.level()));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), milestone.level()));
    ASSERT_THAT(mode, Eq(flashback::practice_mode::aggressive));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), first_card.id(), 20, mode));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), second_card.id(), 20, mode));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), third_card.id(), 20, mode));
    std::vector<flashback::Weight> weights{};
    EXPECT_NO_THROW(weights = m_database->get_progress_weight(m_user->id()));
    EXPECT_THAT(weights, SizeIs(3));
    ASSERT_NO_THROW(weights.at(0).resource());
    ASSERT_NO_THROW(weights.at(1).resource());
    ASSERT_NO_THROW(weights.at(2).resource());
    EXPECT_TRUE(weights.at(0).has_resource());
    EXPECT_TRUE(weights.at(1).has_resource());
    EXPECT_TRUE(weights.at(2).has_resource());
    EXPECT_THAT(weights.at(0).percentage(), Eq(100));
    EXPECT_THAT(weights.at(1).percentage(), Eq(100));
    EXPECT_THAT(weights.at(2).percentage(), Eq(100));
}

TEST_F(test_database, create_assessment)
{
    flashback::Roadmap roadmap{};
    flashback::Subject subject{};
    flashback::Milestone milestone{};
    flashback::Topic first_topic{};
    flashback::Topic second_topic{};
    flashback::Topic third_topic{};
    flashback::Resource resource{};
    flashback::Section section{};
    flashback::Card first_card{};
    flashback::Card second_card{};
    flashback::Card third_card{};
    flashback::practice_mode mode{};

    roadmap.set_name("C++ Software Engineer");
    subject.set_name("C++");
    milestone.set_level(flashback::expertise_level::depth);
    first_topic.set_name("Reflection");
    first_topic.set_level(flashback::expertise_level::surface);
    second_topic.set_name("Coroutine");
    second_topic.set_level(flashback::expertise_level::surface);
    third_topic.set_name("Modules");
    third_topic.set_level(flashback::expertise_level::surface);
    resource.set_name("Personal Knowledge");
    resource.set_type(flashback::Resource::nerve);
    resource.set_pattern(flashback::Resource::synapse);
    section.set_name("First C++ Resource Chapter 1");
    first_card.set_headline("First Card");
    second_card.set_headline("Second Card");
    third_card.set_headline("Third Card");

    ASSERT_NO_THROW(roadmap = m_database->create_roadmap(m_user->id(), roadmap.name()));
    ASSERT_THAT(roadmap.id(), Gt(0));
    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_THAT(subject.id(), Gt(0));
    ASSERT_NO_THROW(milestone = m_database->add_milestone(subject.id(), milestone.level(), roadmap.id()));
    ASSERT_THAT(milestone.position(), Eq(1));
    ASSERT_NO_THROW(first_topic = m_database->create_topic(subject.id(), first_topic.name(), first_topic.level(), 0));
    ASSERT_THAT(first_topic.position(), Eq(1));
    ASSERT_NO_THROW(second_topic = m_database->create_topic(subject.id(), second_topic.name(), second_topic.level(), 0));
    ASSERT_THAT(second_topic.position(), Eq(2));
    ASSERT_NO_THROW(third_topic = m_database->create_topic(subject.id(), third_topic.name(), third_topic.level(), 0));
    ASSERT_THAT(third_topic.position(), Eq(3));
    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_resource_to_subject(resource.id(), subject.id()));
    ASSERT_NO_THROW(section = m_database->create_section(resource.id(), 0, section.name(), section.link()));
    ASSERT_THAT(section.position(), Eq(1));
    ASSERT_NO_THROW(first_card = m_database->create_card(first_card));
    ASSERT_THAT(first_card.id(), Gt(0));
    ASSERT_NO_THROW(second_card = m_database->create_card(second_card));
    ASSERT_THAT(second_card.id(), Gt(0));
    ASSERT_NO_THROW(third_card = m_database->create_card(third_card));
    ASSERT_THAT(third_card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(first_card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_section(second_card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_section(third_card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(first_card.id(), subject.id(), first_topic.position(), first_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(second_card.id(), subject.id(), second_topic.position(), second_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(third_card.id(), subject.id(), third_topic.position(), third_topic.level()));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), milestone.level()));
    ASSERT_THAT(mode, Eq(flashback::practice_mode::aggressive));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), first_card.id(), 20, mode));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), second_card.id(), 20, mode));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), third_card.id(), 20, mode));
    EXPECT_NO_THROW(m_database->create_assessment(subject.id(), first_topic.level(), first_topic.position(), first_card.id()));
    EXPECT_NO_THROW(m_database->create_assessment(subject.id(), first_topic.level(), first_topic.position(), second_card.id()));
}

TEST_F(test_database, expand_assessment)
{
    flashback::Roadmap roadmap{};
    flashback::Subject subject{};
    flashback::Milestone milestone{};
    flashback::Topic first_topic{};
    flashback::Topic second_topic{};
    flashback::Topic third_topic{};
    flashback::Resource resource{};
    flashback::Section section{};
    flashback::Card first_card{};
    flashback::Card second_card{};
    flashback::Card third_card{};
    flashback::practice_mode mode{};

    roadmap.set_name("C++ Software Engineer");
    subject.set_name("C++");
    milestone.set_level(flashback::expertise_level::depth);
    first_topic.set_name("Reflection");
    first_topic.set_level(flashback::expertise_level::surface);
    second_topic.set_name("Coroutine");
    second_topic.set_level(flashback::expertise_level::surface);
    third_topic.set_name("Modules");
    third_topic.set_level(flashback::expertise_level::surface);
    resource.set_name("Personal Knowledge");
    resource.set_type(flashback::Resource::nerve);
    resource.set_pattern(flashback::Resource::synapse);
    section.set_name("First C++ Resource Chapter 1");
    first_card.set_headline("First Card");
    second_card.set_headline("Second Card");
    third_card.set_headline("Third Card");

    ASSERT_NO_THROW(roadmap = m_database->create_roadmap(m_user->id(), roadmap.name()));
    ASSERT_THAT(roadmap.id(), Gt(0));
    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_THAT(subject.id(), Gt(0));
    ASSERT_NO_THROW(milestone = m_database->add_milestone(subject.id(), milestone.level(), roadmap.id()));
    ASSERT_THAT(milestone.position(), Eq(1));
    ASSERT_NO_THROW(first_topic = m_database->create_topic(subject.id(), first_topic.name(), first_topic.level(), 0));
    ASSERT_THAT(first_topic.position(), Eq(1));
    ASSERT_NO_THROW(second_topic = m_database->create_topic(subject.id(), second_topic.name(), second_topic.level(), 0));
    ASSERT_THAT(second_topic.position(), Eq(2));
    ASSERT_NO_THROW(third_topic = m_database->create_topic(subject.id(), third_topic.name(), third_topic.level(), 0));
    ASSERT_THAT(third_topic.position(), Eq(3));
    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_resource_to_subject(resource.id(), subject.id()));
    ASSERT_NO_THROW(section = m_database->create_section(resource.id(), 0, section.name(), section.link()));
    ASSERT_THAT(section.position(), Eq(1));
    ASSERT_NO_THROW(first_card = m_database->create_card(first_card));
    ASSERT_THAT(first_card.id(), Gt(0));
    ASSERT_NO_THROW(second_card = m_database->create_card(second_card));
    ASSERT_THAT(second_card.id(), Gt(0));
    ASSERT_NO_THROW(third_card = m_database->create_card(third_card));
    ASSERT_THAT(third_card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(first_card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_section(second_card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_section(third_card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(first_card.id(), subject.id(), first_topic.position(), first_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(second_card.id(), subject.id(), second_topic.position(), second_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(third_card.id(), subject.id(), third_topic.position(), third_topic.level()));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), milestone.level()));
    ASSERT_THAT(mode, Eq(flashback::practice_mode::aggressive));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), first_card.id(), 20, mode));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), second_card.id(), 20, mode));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), third_card.id(), 20, mode));
    ASSERT_NO_THROW(m_database->create_assessment(subject.id(), first_topic.level(), first_topic.position(), first_card.id()));
    ASSERT_NO_THROW(m_database->create_assessment(subject.id(), first_topic.level(), first_topic.position(), second_card.id()));
    EXPECT_NO_THROW(m_database->expand_assessment(second_card.id(), subject.id(), second_topic.level(), second_topic.position()));
}

TEST_F(test_database, diminish_assessment)
{
    flashback::Roadmap roadmap{};
    flashback::Subject subject{};
    flashback::Milestone milestone{};
    flashback::Topic first_topic{};
    flashback::Topic second_topic{};
    flashback::Topic third_topic{};
    flashback::Resource resource{};
    flashback::Section section{};
    flashback::Card first_card{};
    flashback::Card second_card{};
    flashback::Card third_card{};
    flashback::practice_mode mode{};

    roadmap.set_name("C++ Software Engineer");
    subject.set_name("C++");
    milestone.set_level(flashback::expertise_level::depth);
    first_topic.set_name("Reflection");
    first_topic.set_level(flashback::expertise_level::surface);
    second_topic.set_name("Coroutine");
    second_topic.set_level(flashback::expertise_level::surface);
    third_topic.set_name("Modules");
    third_topic.set_level(flashback::expertise_level::surface);
    resource.set_name("Personal Knowledge");
    resource.set_type(flashback::Resource::nerve);
    resource.set_pattern(flashback::Resource::synapse);
    section.set_name("First C++ Resource Chapter 1");
    first_card.set_headline("First Card");
    second_card.set_headline("Second Card");
    third_card.set_headline("Third Card");

    ASSERT_NO_THROW(roadmap = m_database->create_roadmap(m_user->id(), roadmap.name()));
    ASSERT_THAT(roadmap.id(), Gt(0));
    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_THAT(subject.id(), Gt(0));
    ASSERT_NO_THROW(milestone = m_database->add_milestone(subject.id(), milestone.level(), roadmap.id()));
    ASSERT_THAT(milestone.position(), Eq(1));
    ASSERT_NO_THROW(first_topic = m_database->create_topic(subject.id(), first_topic.name(), first_topic.level(), 0));
    ASSERT_THAT(first_topic.position(), Eq(1));
    ASSERT_NO_THROW(second_topic = m_database->create_topic(subject.id(), second_topic.name(), second_topic.level(), 0));
    ASSERT_THAT(second_topic.position(), Eq(2));
    ASSERT_NO_THROW(third_topic = m_database->create_topic(subject.id(), third_topic.name(), third_topic.level(), 0));
    ASSERT_THAT(third_topic.position(), Eq(3));
    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_resource_to_subject(resource.id(), subject.id()));
    ASSERT_NO_THROW(section = m_database->create_section(resource.id(), 0, section.name(), section.link()));
    ASSERT_THAT(section.position(), Eq(1));
    ASSERT_NO_THROW(first_card = m_database->create_card(first_card));
    ASSERT_THAT(first_card.id(), Gt(0));
    ASSERT_NO_THROW(second_card = m_database->create_card(second_card));
    ASSERT_THAT(second_card.id(), Gt(0));
    ASSERT_NO_THROW(third_card = m_database->create_card(third_card));
    ASSERT_THAT(third_card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(first_card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_section(second_card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_section(third_card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(first_card.id(), subject.id(), first_topic.position(), first_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(second_card.id(), subject.id(), second_topic.position(), second_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(third_card.id(), subject.id(), third_topic.position(), third_topic.level()));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), milestone.level()));
    ASSERT_THAT(mode, Eq(flashback::practice_mode::aggressive));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), first_card.id(), 20, mode));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), second_card.id(), 20, mode));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), third_card.id(), 20, mode));
    ASSERT_NO_THROW(m_database->create_assessment(subject.id(), first_topic.level(), first_topic.position(), first_card.id()));
    ASSERT_NO_THROW(m_database->create_assessment(subject.id(), first_topic.level(), first_topic.position(), second_card.id()));
    EXPECT_NO_THROW(m_database->expand_assessment(second_card.id(), subject.id(), second_topic.level(), second_topic.position()));
    EXPECT_NO_THROW(m_database->diminish_assessment(second_card.id(), subject.id(), first_topic.level(), first_topic.position()));
}

TEST_F(test_database, get_topic_coverage)
{
    flashback::Roadmap roadmap{};
    flashback::Subject subject{};
    flashback::Milestone milestone{};
    flashback::Topic first_topic{};
    flashback::Topic second_topic{};
    flashback::Topic third_topic{};
    flashback::Resource resource{};
    flashback::Section section{};
    flashback::Card first_card{};
    flashback::Card second_card{};
    flashback::Card third_card{};
    flashback::practice_mode mode{};

    roadmap.set_name("C++ Software Engineer");
    subject.set_name("C++");
    milestone.set_level(flashback::expertise_level::depth);
    first_topic.set_name("Reflection");
    first_topic.set_level(flashback::expertise_level::surface);
    second_topic.set_name("Coroutine");
    second_topic.set_level(flashback::expertise_level::surface);
    third_topic.set_name("Modules");
    third_topic.set_level(flashback::expertise_level::surface);
    resource.set_name("Personal Knowledge");
    resource.set_type(flashback::Resource::nerve);
    resource.set_pattern(flashback::Resource::synapse);
    section.set_name("First C++ Resource Chapter 1");
    first_card.set_headline("First Card");
    second_card.set_headline("Second Card");
    third_card.set_headline("Third Card");

    ASSERT_NO_THROW(roadmap = m_database->create_roadmap(m_user->id(), roadmap.name()));
    ASSERT_THAT(roadmap.id(), Gt(0));
    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_THAT(subject.id(), Gt(0));
    ASSERT_NO_THROW(milestone = m_database->add_milestone(subject.id(), milestone.level(), roadmap.id()));
    ASSERT_THAT(milestone.position(), Eq(1));
    ASSERT_NO_THROW(first_topic = m_database->create_topic(subject.id(), first_topic.name(), first_topic.level(), 0));
    ASSERT_THAT(first_topic.position(), Eq(1));
    ASSERT_NO_THROW(second_topic = m_database->create_topic(subject.id(), second_topic.name(), second_topic.level(), 0));
    ASSERT_THAT(second_topic.position(), Eq(2));
    ASSERT_NO_THROW(third_topic = m_database->create_topic(subject.id(), third_topic.name(), third_topic.level(), 0));
    ASSERT_THAT(third_topic.position(), Eq(3));
    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_resource_to_subject(resource.id(), subject.id()));
    ASSERT_NO_THROW(section = m_database->create_section(resource.id(), 0, section.name(), section.link()));
    ASSERT_THAT(section.position(), Eq(1));
    ASSERT_NO_THROW(first_card = m_database->create_card(first_card));
    ASSERT_THAT(first_card.id(), Gt(0));
    ASSERT_NO_THROW(second_card = m_database->create_card(second_card));
    ASSERT_THAT(second_card.id(), Gt(0));
    ASSERT_NO_THROW(third_card = m_database->create_card(third_card));
    ASSERT_THAT(third_card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(first_card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_section(second_card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_section(third_card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(first_card.id(), subject.id(), first_topic.position(), first_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(second_card.id(), subject.id(), second_topic.position(), second_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(third_card.id(), subject.id(), third_topic.position(), third_topic.level()));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), milestone.level()));
    ASSERT_THAT(mode, Eq(flashback::practice_mode::aggressive));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), first_card.id(), 20, mode));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), second_card.id(), 20, mode));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), third_card.id(), 20, mode));
    EXPECT_NO_THROW(m_database->create_assessment(subject.id(), first_topic.level(), first_topic.position(), first_card.id()));
    EXPECT_NO_THROW(m_database->create_assessment(subject.id(), first_topic.level(), first_topic.position(), second_card.id()));
    std::vector<flashback::Topic> topics{};
    EXPECT_NO_THROW(topics = m_database->get_topic_coverage(subject.id(), first_card.id()));
    EXPECT_THAT(topics, SizeIs(1));
    EXPECT_NO_THROW(topics = m_database->get_topic_coverage(subject.id(), second_card.id()));
    EXPECT_THAT(topics, SizeIs(1));
    EXPECT_NO_THROW(m_database->expand_assessment(second_card.id(), subject.id(), second_topic.level(), second_topic.position()));
    EXPECT_NO_THROW(topics = m_database->get_topic_coverage(subject.id(), first_card.id()));
    EXPECT_THAT(topics, SizeIs(1));
    EXPECT_NO_THROW(topics.at(0).position());
    EXPECT_NO_THROW(topics = m_database->get_topic_coverage(subject.id(), second_card.id()));
    EXPECT_THAT(topics, SizeIs(2));
    EXPECT_NO_THROW(topics.at(0).position());
    EXPECT_NO_THROW(topics.at(1).position());
    EXPECT_NO_THROW(m_database->diminish_assessment(second_card.id(), subject.id(), first_topic.level(), first_topic.position()));
    EXPECT_NO_THROW(topics = m_database->get_topic_coverage(subject.id(), first_card.id()));
    EXPECT_THAT(topics, SizeIs(1));
    EXPECT_NO_THROW(topics.at(0).position());
    EXPECT_NO_THROW(topics = m_database->get_topic_coverage(subject.id(), second_card.id()));
    EXPECT_THAT(topics, SizeIs(1));
    EXPECT_NO_THROW(topics.at(0).position());
}

TEST_F(test_database, get_assessment_coverage)
{
    flashback::Roadmap roadmap{};
    flashback::Subject subject{};
    flashback::Milestone milestone{};
    flashback::Topic first_topic{};
    flashback::Topic second_topic{};
    flashback::Topic third_topic{};
    flashback::Resource resource{};
    flashback::Section section{};
    flashback::Card first_card{};
    flashback::Card second_card{};
    flashback::Card third_card{};
    flashback::practice_mode mode{};

    roadmap.set_name("C++ Software Engineer");
    subject.set_name("C++");
    milestone.set_level(flashback::expertise_level::depth);
    first_topic.set_name("Reflection");
    first_topic.set_level(flashback::expertise_level::surface);
    second_topic.set_name("Coroutine");
    second_topic.set_level(flashback::expertise_level::surface);
    third_topic.set_name("Modules");
    third_topic.set_level(flashback::expertise_level::surface);
    resource.set_name("Personal Knowledge");
    resource.set_type(flashback::Resource::nerve);
    resource.set_pattern(flashback::Resource::synapse);
    section.set_name("First C++ Resource Chapter 1");
    first_card.set_headline("First Card");
    second_card.set_headline("Second Card");
    third_card.set_headline("Third Card");

    ASSERT_NO_THROW(roadmap = m_database->create_roadmap(m_user->id(), roadmap.name()));
    ASSERT_THAT(roadmap.id(), Gt(0));
    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_THAT(subject.id(), Gt(0));
    ASSERT_NO_THROW(milestone = m_database->add_milestone(subject.id(), milestone.level(), roadmap.id()));
    ASSERT_THAT(milestone.position(), Eq(1));
    ASSERT_NO_THROW(first_topic = m_database->create_topic(subject.id(), first_topic.name(), first_topic.level(), 0));
    ASSERT_THAT(first_topic.position(), Eq(1));
    ASSERT_NO_THROW(second_topic = m_database->create_topic(subject.id(), second_topic.name(), second_topic.level(), 0));
    ASSERT_THAT(second_topic.position(), Eq(2));
    ASSERT_NO_THROW(third_topic = m_database->create_topic(subject.id(), third_topic.name(), third_topic.level(), 0));
    ASSERT_THAT(third_topic.position(), Eq(3));
    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_resource_to_subject(resource.id(), subject.id()));
    ASSERT_NO_THROW(section = m_database->create_section(resource.id(), 0, section.name(), section.link()));
    ASSERT_THAT(section.position(), Eq(1));
    ASSERT_NO_THROW(first_card = m_database->create_card(first_card));
    ASSERT_THAT(first_card.id(), Gt(0));
    ASSERT_NO_THROW(second_card = m_database->create_card(second_card));
    ASSERT_THAT(second_card.id(), Gt(0));
    ASSERT_NO_THROW(third_card = m_database->create_card(third_card));
    ASSERT_THAT(third_card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(first_card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_section(second_card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_section(third_card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(first_card.id(), subject.id(), first_topic.position(), first_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(second_card.id(), subject.id(), second_topic.position(), second_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(third_card.id(), subject.id(), third_topic.position(), third_topic.level()));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), milestone.level()));
    ASSERT_THAT(mode, Eq(flashback::practice_mode::aggressive));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), first_card.id(), 20, mode));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), second_card.id(), 20, mode));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), third_card.id(), 20, mode));
    EXPECT_NO_THROW(m_database->create_assessment(subject.id(), first_topic.level(), first_topic.position(), first_card.id()));
    EXPECT_NO_THROW(m_database->create_assessment(subject.id(), first_topic.level(), first_topic.position(), second_card.id()));
    EXPECT_NO_THROW(m_database->create_assessment(subject.id(), third_topic.level(), third_topic.position(), third_card.id()));
    std::vector<flashback::Coverage> coverage{};
    EXPECT_NO_THROW(coverage = m_database->get_assessment_coverage(subject.id(), first_topic.position(), first_topic.level()));
    EXPECT_THAT(coverage, SizeIs(2));
    EXPECT_NO_THROW(coverage = m_database->get_assessment_coverage(subject.id(), second_topic.position(), second_topic.level()));
    EXPECT_THAT(coverage, IsEmpty());
    EXPECT_NO_THROW(coverage = m_database->get_assessment_coverage(subject.id(), third_topic.position(), third_topic.level()));
    EXPECT_THAT(coverage, SizeIs(1));
}

TEST_F(test_database, get_assimilation_coverage)
{
    flashback::Roadmap roadmap{};
    flashback::Subject subject{};
    flashback::Milestone milestone{};
    flashback::Topic first_topic{};
    flashback::Topic second_topic{};
    flashback::Topic third_topic{};
    flashback::Resource resource{};
    flashback::Section section{};
    flashback::Card first_card{};
    flashback::Card second_card{};
    flashback::Card third_card{};
    flashback::practice_mode mode{};

    roadmap.set_name("C++ Software Engineer");
    subject.set_name("C++");
    milestone.set_level(flashback::expertise_level::depth);
    first_topic.set_name("Reflection");
    first_topic.set_level(flashback::expertise_level::surface);
    second_topic.set_name("Coroutine");
    second_topic.set_level(flashback::expertise_level::surface);
    third_topic.set_name("Modules");
    third_topic.set_level(flashback::expertise_level::surface);
    resource.set_name("Personal Knowledge");
    resource.set_type(flashback::Resource::nerve);
    resource.set_pattern(flashback::Resource::synapse);
    section.set_name("First C++ Resource Chapter 1");
    first_card.set_headline("First Card");
    second_card.set_headline("Second Card");
    third_card.set_headline("Third Card");

    ASSERT_NO_THROW(roadmap = m_database->create_roadmap(m_user->id(), roadmap.name()));
    ASSERT_THAT(roadmap.id(), Gt(0));
    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_THAT(subject.id(), Gt(0));
    ASSERT_NO_THROW(milestone = m_database->add_milestone(subject.id(), milestone.level(), roadmap.id()));
    ASSERT_THAT(milestone.position(), Eq(1));
    ASSERT_NO_THROW(first_topic = m_database->create_topic(subject.id(), first_topic.name(), first_topic.level(), 0));
    ASSERT_THAT(first_topic.position(), Eq(1));
    ASSERT_NO_THROW(second_topic = m_database->create_topic(subject.id(), second_topic.name(), second_topic.level(), 0));
    ASSERT_THAT(second_topic.position(), Eq(2));
    ASSERT_NO_THROW(third_topic = m_database->create_topic(subject.id(), third_topic.name(), third_topic.level(), 0));
    ASSERT_THAT(third_topic.position(), Eq(3));
    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_resource_to_subject(resource.id(), subject.id()));
    ASSERT_NO_THROW(section = m_database->create_section(resource.id(), 0, section.name(), section.link()));
    ASSERT_THAT(section.position(), Eq(1));
    ASSERT_NO_THROW(first_card = m_database->create_card(first_card));
    ASSERT_THAT(first_card.id(), Gt(0));
    ASSERT_NO_THROW(second_card = m_database->create_card(second_card));
    ASSERT_THAT(second_card.id(), Gt(0));
    ASSERT_NO_THROW(third_card = m_database->create_card(third_card));
    ASSERT_THAT(third_card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(first_card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_section(second_card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_section(third_card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(first_card.id(), subject.id(), first_topic.position(), first_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(second_card.id(), subject.id(), second_topic.position(), second_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(third_card.id(), subject.id(), third_topic.position(), third_topic.level()));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), milestone.level()));
    ASSERT_THAT(mode, Eq(flashback::practice_mode::aggressive));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), first_card.id(), 20, mode));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), second_card.id(), 20, mode));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), third_card.id(), 20, mode));
    EXPECT_NO_THROW(m_database->create_assessment(subject.id(), first_topic.level(), first_topic.position(), first_card.id()));
    EXPECT_NO_THROW(m_database->create_assessment(subject.id(), first_topic.level(), first_topic.position(), second_card.id()));
    EXPECT_NO_THROW(m_database->create_assessment(subject.id(), third_topic.level(), third_topic.position(), third_card.id()));
    std::vector<flashback::Coverage> coverage{};
    EXPECT_NO_THROW(coverage = m_database->get_assessment_coverage(subject.id(), first_topic.position(), first_topic.level()));
    EXPECT_THAT(coverage, SizeIs(2));
    EXPECT_NO_THROW(coverage = m_database->get_assessment_coverage(subject.id(), second_topic.position(), second_topic.level()));
    EXPECT_THAT(coverage, IsEmpty());
    EXPECT_NO_THROW(coverage = m_database->get_assessment_coverage(subject.id(), third_topic.position(), third_topic.level()));
    EXPECT_THAT(coverage, SizeIs(1));
    std::map<uint64_t, flashback::Assimilation> assimilation_coverage{};
    EXPECT_NO_THROW(assimilation_coverage = m_database->get_assimilation_coverage(m_user->id(), subject.id(), first_card.id()));
    EXPECT_THAT(assimilation_coverage, SizeIs(1));
    EXPECT_NO_THROW(assimilation_coverage = m_database->get_assimilation_coverage(m_user->id(), subject.id(), second_card.id()));
    EXPECT_THAT(assimilation_coverage, SizeIs(1));
    EXPECT_NO_THROW(assimilation_coverage = m_database->get_assimilation_coverage(m_user->id(), subject.id(), third_card.id()));
    EXPECT_THAT(assimilation_coverage, SizeIs(1));
    EXPECT_NO_THROW(m_database->expand_assessment(second_card.id(), subject.id(), second_topic.level(), second_topic.position()));
    EXPECT_NO_THROW(assimilation_coverage = m_database->get_assimilation_coverage(m_user->id(), subject.id(), first_card.id()));
    EXPECT_THAT(assimilation_coverage, SizeIs(1));
    EXPECT_NO_THROW(assimilation_coverage = m_database->get_assimilation_coverage(m_user->id(), subject.id(), second_card.id()));
    EXPECT_THAT(assimilation_coverage, SizeIs(2));
    EXPECT_NO_THROW(assimilation_coverage = m_database->get_assimilation_coverage(m_user->id(), subject.id(), third_card.id()));
    EXPECT_THAT(assimilation_coverage, SizeIs(1));
    EXPECT_NO_THROW(m_database->expand_assessment(third_card.id(), subject.id(), first_topic.level(), first_topic.position()));
    EXPECT_NO_THROW(m_database->expand_assessment(third_card.id(), subject.id(), second_topic.level(), second_topic.position()));
    EXPECT_NO_THROW(assimilation_coverage = m_database->get_assimilation_coverage(m_user->id(), subject.id(), first_card.id()));
    EXPECT_THAT(assimilation_coverage, SizeIs(1));
    EXPECT_NO_THROW(assimilation_coverage = m_database->get_assimilation_coverage(m_user->id(), subject.id(), second_card.id()));
    EXPECT_THAT(assimilation_coverage, SizeIs(2));
    EXPECT_NO_THROW(assimilation_coverage = m_database->get_assimilation_coverage(m_user->id(), subject.id(), third_card.id()));
    EXPECT_THAT(assimilation_coverage, SizeIs(3));
}

TEST_F(test_database, get_topic_assessments)
{
    flashback::Roadmap roadmap{};
    flashback::Subject subject{};
    flashback::Milestone milestone{};
    flashback::Topic first_topic{};
    flashback::Topic second_topic{};
    flashback::Topic third_topic{};
    flashback::Resource resource{};
    flashback::Section section{};
    flashback::Card first_card{};
    flashback::Card second_card{};
    flashback::Card third_card{};
    flashback::practice_mode mode{};

    roadmap.set_name("C++ Software Engineer");
    subject.set_name("C++");
    milestone.set_level(flashback::expertise_level::depth);
    first_topic.set_name("Reflection");
    first_topic.set_level(flashback::expertise_level::surface);
    second_topic.set_name("Coroutine");
    second_topic.set_level(flashback::expertise_level::surface);
    third_topic.set_name("Modules");
    third_topic.set_level(flashback::expertise_level::surface);
    resource.set_name("Personal Knowledge");
    resource.set_type(flashback::Resource::nerve);
    resource.set_pattern(flashback::Resource::synapse);
    section.set_name("First C++ Resource Chapter 1");
    first_card.set_headline("First Card");
    second_card.set_headline("Second Card");
    third_card.set_headline("Third Card");

    ASSERT_NO_THROW(roadmap = m_database->create_roadmap(m_user->id(), roadmap.name()));
    ASSERT_THAT(roadmap.id(), Gt(0));
    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_THAT(subject.id(), Gt(0));
    ASSERT_NO_THROW(milestone = m_database->add_milestone(subject.id(), milestone.level(), roadmap.id()));
    ASSERT_THAT(milestone.position(), Eq(1));
    ASSERT_NO_THROW(first_topic = m_database->create_topic(subject.id(), first_topic.name(), first_topic.level(), 0));
    ASSERT_THAT(first_topic.position(), Eq(1));
    ASSERT_NO_THROW(second_topic = m_database->create_topic(subject.id(), second_topic.name(), second_topic.level(), 0));
    ASSERT_THAT(second_topic.position(), Eq(2));
    ASSERT_NO_THROW(third_topic = m_database->create_topic(subject.id(), third_topic.name(), third_topic.level(), 0));
    ASSERT_THAT(third_topic.position(), Eq(3));
    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_resource_to_subject(resource.id(), subject.id()));
    ASSERT_NO_THROW(section = m_database->create_section(resource.id(), 0, section.name(), section.link()));
    ASSERT_THAT(section.position(), Eq(1));
    ASSERT_NO_THROW(first_card = m_database->create_card(first_card));
    ASSERT_THAT(first_card.id(), Gt(0));
    ASSERT_NO_THROW(second_card = m_database->create_card(second_card));
    ASSERT_THAT(second_card.id(), Gt(0));
    ASSERT_NO_THROW(third_card = m_database->create_card(third_card));
    ASSERT_THAT(third_card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(first_card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_section(second_card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_section(third_card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(first_card.id(), subject.id(), first_topic.position(), first_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(second_card.id(), subject.id(), second_topic.position(), second_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(third_card.id(), subject.id(), third_topic.position(), third_topic.level()));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), milestone.level()));
    ASSERT_THAT(mode, Eq(flashback::practice_mode::aggressive));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), first_card.id(), 20, mode));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), second_card.id(), 20, mode));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), third_card.id(), 20, mode));
    EXPECT_NO_THROW(m_database->create_assessment(subject.id(), first_topic.level(), first_topic.position(), first_card.id()));
    EXPECT_NO_THROW(m_database->create_assessment(subject.id(), first_topic.level(), first_topic.position(), second_card.id()));
    EXPECT_NO_THROW(m_database->create_assessment(subject.id(), third_topic.level(), third_topic.position(), third_card.id()));
    std::vector<flashback::Card> assessments{};
    EXPECT_NO_THROW(assessments = m_database->get_topic_assessments(m_user->id(), subject.id(), first_topic.position(), first_topic.level()));
    EXPECT_THAT(assessments, SizeIs(2));
    EXPECT_NO_THROW(assessments = m_database->get_topic_assessments(m_user->id(), subject.id(), third_topic.position(), third_topic.level()));
    EXPECT_THAT(assessments, SizeIs(1));
}

TEST_F(test_database, get_assessments)
{
    flashback::Roadmap roadmap{};
    flashback::Subject subject{};
    flashback::Milestone milestone{};
    flashback::Topic first_topic{};
    flashback::Topic second_topic{};
    flashback::Topic third_topic{};
    flashback::Resource resource{};
    flashback::Section section{};
    flashback::Card first_card{};
    flashback::Card second_card{};
    flashback::Card third_card{};
    flashback::practice_mode mode{};

    roadmap.set_name("C++ Software Engineer");
    subject.set_name("C++");
    milestone.set_level(flashback::expertise_level::depth);
    first_topic.set_name("Reflection");
    first_topic.set_level(flashback::expertise_level::surface);
    second_topic.set_name("Coroutine");
    second_topic.set_level(flashback::expertise_level::surface);
    third_topic.set_name("Modules");
    third_topic.set_level(flashback::expertise_level::surface);
    resource.set_name("Personal Knowledge");
    resource.set_type(flashback::Resource::nerve);
    resource.set_pattern(flashback::Resource::synapse);
    section.set_name("First C++ Resource Chapter 1");
    first_card.set_headline("First Card");
    second_card.set_headline("Second Card");
    third_card.set_headline("Third Card");

    ASSERT_NO_THROW(roadmap = m_database->create_roadmap(m_user->id(), roadmap.name()));
    ASSERT_THAT(roadmap.id(), Gt(0));
    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_THAT(subject.id(), Gt(0));
    ASSERT_NO_THROW(milestone = m_database->add_milestone(subject.id(), milestone.level(), roadmap.id()));
    ASSERT_THAT(milestone.position(), Eq(1));
    ASSERT_NO_THROW(first_topic = m_database->create_topic(subject.id(), first_topic.name(), first_topic.level(), 0));
    ASSERT_THAT(first_topic.position(), Eq(1));
    ASSERT_NO_THROW(second_topic = m_database->create_topic(subject.id(), second_topic.name(), second_topic.level(), 0));
    ASSERT_THAT(second_topic.position(), Eq(2));
    ASSERT_NO_THROW(third_topic = m_database->create_topic(subject.id(), third_topic.name(), third_topic.level(), 0));
    ASSERT_THAT(third_topic.position(), Eq(3));
    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_resource_to_subject(resource.id(), subject.id()));
    ASSERT_NO_THROW(section = m_database->create_section(resource.id(), 0, section.name(), section.link()));
    ASSERT_THAT(section.position(), Eq(1));
    ASSERT_NO_THROW(first_card = m_database->create_card(first_card));
    ASSERT_THAT(first_card.id(), Gt(0));
    ASSERT_NO_THROW(second_card = m_database->create_card(second_card));
    ASSERT_THAT(second_card.id(), Gt(0));
    ASSERT_NO_THROW(third_card = m_database->create_card(third_card));
    ASSERT_THAT(third_card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(first_card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_section(second_card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_section(third_card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(first_card.id(), subject.id(), first_topic.position(), first_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(second_card.id(), subject.id(), second_topic.position(), second_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(third_card.id(), subject.id(), third_topic.position(), third_topic.level()));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::surface));
    ASSERT_THAT(mode, Eq(flashback::practice_mode::aggressive));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::depth));
    ASSERT_THAT(mode, Eq(flashback::practice_mode::aggressive));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::origin));
    ASSERT_THAT(mode, Eq(flashback::practice_mode::aggressive));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), first_card.id(), 20, mode));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), second_card.id(), 20, mode));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), third_card.id(), 20, mode));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::surface));
    ASSERT_THAT(mode, Eq(flashback::practice_mode::progressive));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::depth));
    ASSERT_THAT(mode, Eq(flashback::practice_mode::progressive));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::origin));
    ASSERT_THAT(mode, Eq(flashback::practice_mode::progressive));
    EXPECT_NO_THROW(m_database->create_assessment(subject.id(), first_topic.level(), first_topic.position(), first_card.id()));
    EXPECT_NO_THROW(m_database->create_assessment(subject.id(), second_topic.level(), second_topic.position(), second_card.id()));
    EXPECT_NO_THROW(m_database->create_assessment(subject.id(), third_topic.level(), third_topic.position(), third_card.id()));
    std::vector<flashback::Assessment> assessments{};
    EXPECT_NO_THROW(assessments = m_database->get_assessments(m_user->id(), subject.id(), first_topic.level(), first_topic.position()));
    EXPECT_THAT(assessments, SizeIs(1));
    EXPECT_NO_THROW(assessments = m_database->get_assessments(m_user->id(), subject.id(), second_topic.level(), second_topic.position()));
    EXPECT_THAT(assessments, SizeIs(1));
    EXPECT_NO_THROW(assessments = m_database->get_assessments(m_user->id(), subject.id(), third_topic.level(), third_topic.position()));
    EXPECT_THAT(assessments, SizeIs(1));
    std::vector<flashback::Coverage> coverage{};
    EXPECT_NO_THROW(coverage = m_database->get_assessment_coverage(subject.id(), first_topic.position(), first_topic.level()));
    EXPECT_THAT(coverage, SizeIs(1));
    ASSERT_NO_THROW(coverage.at(0).card().id());
    ASSERT_THAT(coverage.at(0).card().id(), Eq(first_card.id()));
    EXPECT_THAT(coverage.at(0).coverage(), Eq(1));
    EXPECT_NO_THROW(coverage = m_database->get_assessment_coverage(subject.id(), second_topic.position(), second_topic.level()));
    EXPECT_THAT(coverage, SizeIs(1));
    ASSERT_NO_THROW(coverage.at(0).card().id());
    ASSERT_THAT(coverage.at(0).card().id(), Eq(second_card.id()));
    EXPECT_THAT(coverage.at(0).coverage(), Eq(1));
    EXPECT_NO_THROW(coverage = m_database->get_assessment_coverage(subject.id(), third_topic.position(), third_topic.level()));
    EXPECT_THAT(coverage, SizeIs(1));
    ASSERT_NO_THROW(coverage.at(0).card().id());
    EXPECT_THAT(coverage.at(0).card().id(), third_card.id());
    EXPECT_THAT(coverage.at(0).coverage(), Eq(1));
    std::map<uint64_t, flashback::Assimilation> assimilation_coverage{};
    EXPECT_NO_THROW(assimilation_coverage = m_database->get_assimilation_coverage(m_user->id(), subject.id(), first_card.id()));
    EXPECT_THAT(assimilation_coverage, SizeIs(1));
    ASSERT_NO_THROW(assimilation_coverage.at(1).assimilated());
    EXPECT_FALSE(assimilation_coverage.at(1).assimilated());
    EXPECT_NO_THROW(assimilation_coverage = m_database->get_assimilation_coverage(m_user->id(), subject.id(), second_card.id()));
    EXPECT_THAT(assimilation_coverage, SizeIs(1));
    ASSERT_NO_THROW(assimilation_coverage.at(2).assimilated());
    EXPECT_FALSE(assimilation_coverage.at(2).assimilated());
    EXPECT_NO_THROW(assimilation_coverage = m_database->get_assimilation_coverage(m_user->id(), subject.id(), third_card.id()));
    EXPECT_THAT(assimilation_coverage, SizeIs(1));
    ASSERT_NO_THROW(assimilation_coverage.at(3).assimilated());
    EXPECT_FALSE(assimilation_coverage.at(3).assimilated());
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), first_card.id(), 20, mode));
    EXPECT_NO_THROW(throw_back_progress(m_user->id(), first_card.id(), std::chrono::days(4)));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), first_card.id(), 20, mode));
    EXPECT_NO_THROW(throw_back_progress(m_user->id(), first_card.id(), std::chrono::days(3)));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), first_card.id(), 20, mode));
    EXPECT_NO_THROW(throw_back_progress(m_user->id(), first_card.id(), std::chrono::days(2)));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), first_card.id(), 20, mode));
    EXPECT_NO_THROW(throw_back_progress(m_user->id(), first_card.id(), std::chrono::days(1)));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), second_card.id(), 20, mode));
    EXPECT_NO_THROW(throw_back_progress(m_user->id(), second_card.id(), std::chrono::days(4)));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), second_card.id(), 20, mode));
    EXPECT_NO_THROW(throw_back_progress(m_user->id(), second_card.id(), std::chrono::days(3)));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), second_card.id(), 20, mode));
    EXPECT_NO_THROW(throw_back_progress(m_user->id(), second_card.id(), std::chrono::days(2)));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), second_card.id(), 20, mode));
    EXPECT_NO_THROW(throw_back_progress(m_user->id(), second_card.id(), std::chrono::days(1)));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), third_card.id(), 20, mode));
    EXPECT_NO_THROW(throw_back_progress(m_user->id(), third_card.id(), std::chrono::days(4)));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), third_card.id(), 20, mode));
    EXPECT_NO_THROW(throw_back_progress(m_user->id(), third_card.id(), std::chrono::days(3)));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), third_card.id(), 20, mode));
    EXPECT_NO_THROW(throw_back_progress(m_user->id(), third_card.id(), std::chrono::days(2)));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), third_card.id(), 20, mode));
    EXPECT_NO_THROW(throw_back_progress(m_user->id(), third_card.id(), std::chrono::days(1)));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::surface));
    ASSERT_THAT(mode, Eq(flashback::practice_mode::progressive));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::depth));
    ASSERT_THAT(mode, Eq(flashback::practice_mode::progressive));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::origin));
    ASSERT_THAT(mode, Eq(flashback::practice_mode::progressive));
    EXPECT_NO_THROW(assimilation_coverage = m_database->get_assimilation_coverage(m_user->id(), subject.id(), first_card.id()));
    EXPECT_THAT(assimilation_coverage, SizeIs(1));
    ASSERT_NO_THROW(assimilation_coverage.at(1).assimilated());
    EXPECT_TRUE(assimilation_coverage.at(1).assimilated());
    EXPECT_NO_THROW(assimilation_coverage = m_database->get_assimilation_coverage(m_user->id(), subject.id(), second_card.id()));
    EXPECT_THAT(assimilation_coverage, SizeIs(1));
    ASSERT_NO_THROW(assimilation_coverage.at(2).assimilated());
    EXPECT_TRUE(assimilation_coverage.at(2).assimilated());
    EXPECT_NO_THROW(assimilation_coverage = m_database->get_assimilation_coverage(m_user->id(), subject.id(), third_card.id()));
    EXPECT_THAT(assimilation_coverage, SizeIs(1));
    ASSERT_NO_THROW(assimilation_coverage.at(3).assimilated());
    EXPECT_TRUE(assimilation_coverage.at(3).assimilated());
}

TEST_F(test_database, is_assimilated)
{
    flashback::Roadmap roadmap{};
    flashback::Subject subject{};
    flashback::Milestone milestone{};
    flashback::Topic first_topic{};
    flashback::Topic second_topic{};
    flashback::Topic third_topic{};
    flashback::Resource resource{};
    flashback::Section section{};
    flashback::Card first_card{};
    flashback::Card second_card{};
    flashback::Card third_card{};
    flashback::practice_mode mode{};

    roadmap.set_name("C++ Software Engineer");
    subject.set_name("C++");
    milestone.set_level(flashback::expertise_level::depth);
    first_topic.set_name("Reflection");
    first_topic.set_level(flashback::expertise_level::surface);
    second_topic.set_name("Coroutine");
    second_topic.set_level(flashback::expertise_level::surface);
    third_topic.set_name("Modules");
    third_topic.set_level(flashback::expertise_level::surface);
    resource.set_name("Personal Knowledge");
    resource.set_type(flashback::Resource::nerve);
    resource.set_pattern(flashback::Resource::synapse);
    section.set_name("First C++ Resource Chapter 1");
    first_card.set_headline("First Card");
    second_card.set_headline("Second Card");
    third_card.set_headline("Third Card");

    ASSERT_NO_THROW(roadmap = m_database->create_roadmap(m_user->id(), roadmap.name()));
    ASSERT_THAT(roadmap.id(), Gt(0));
    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_THAT(subject.id(), Gt(0));
    ASSERT_NO_THROW(milestone = m_database->add_milestone(subject.id(), milestone.level(), roadmap.id()));
    ASSERT_THAT(milestone.position(), Eq(1));
    ASSERT_NO_THROW(first_topic = m_database->create_topic(subject.id(), first_topic.name(), first_topic.level(), 0));
    ASSERT_THAT(first_topic.position(), Eq(1));
    ASSERT_NO_THROW(second_topic = m_database->create_topic(subject.id(), second_topic.name(), second_topic.level(), 0));
    ASSERT_THAT(second_topic.position(), Eq(2));
    ASSERT_NO_THROW(third_topic = m_database->create_topic(subject.id(), third_topic.name(), third_topic.level(), 0));
    ASSERT_THAT(third_topic.position(), Eq(3));
    ASSERT_NO_THROW(resource = m_database->create_resource(resource));
    ASSERT_THAT(resource.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_resource_to_subject(resource.id(), subject.id()));
    ASSERT_NO_THROW(section = m_database->create_section(resource.id(), 0, section.name(), section.link()));
    ASSERT_THAT(section.position(), Eq(1));
    ASSERT_NO_THROW(first_card = m_database->create_card(first_card));
    ASSERT_THAT(first_card.id(), Gt(0));
    ASSERT_NO_THROW(second_card = m_database->create_card(second_card));
    ASSERT_THAT(second_card.id(), Gt(0));
    ASSERT_NO_THROW(third_card = m_database->create_card(third_card));
    ASSERT_THAT(third_card.id(), Gt(0));
    ASSERT_NO_THROW(m_database->add_card_to_section(first_card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_section(second_card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_section(third_card.id(), resource.id(), section.position()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(first_card.id(), subject.id(), first_topic.position(), first_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(second_card.id(), subject.id(), second_topic.position(), second_topic.level()));
    ASSERT_NO_THROW(m_database->add_card_to_topic(third_card.id(), subject.id(), third_topic.position(), third_topic.level()));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::surface));
    ASSERT_THAT(mode, Eq(flashback::practice_mode::aggressive));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::depth));
    ASSERT_THAT(mode, Eq(flashback::practice_mode::aggressive));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::origin));
    ASSERT_THAT(mode, Eq(flashback::practice_mode::aggressive));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), first_card.id(), 20, mode));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), second_card.id(), 20, mode));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), third_card.id(), 20, mode));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::surface));
    ASSERT_THAT(mode, Eq(flashback::practice_mode::progressive));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::depth));
    ASSERT_THAT(mode, Eq(flashback::practice_mode::progressive));
    ASSERT_NO_THROW(mode = m_database->get_practice_mode(m_user->id(), subject.id(), flashback::expertise_level::origin));
    ASSERT_THAT(mode, Eq(flashback::practice_mode::progressive));
    EXPECT_NO_THROW(m_database->create_assessment(subject.id(), first_topic.level(), first_topic.position(), first_card.id()));
    EXPECT_NO_THROW(m_database->create_assessment(subject.id(), second_topic.level(), second_topic.position(), second_card.id()));
    EXPECT_NO_THROW(m_database->create_assessment(subject.id(), third_topic.level(), third_topic.position(), third_card.id()));
    std::vector<flashback::Assessment> assessments{};
    EXPECT_NO_THROW(assessments = m_database->get_assessments(m_user->id(), subject.id(), first_topic.level(), first_topic.position()));
    EXPECT_THAT(assessments, SizeIs(1));
    EXPECT_NO_THROW(assessments = m_database->get_assessments(m_user->id(), subject.id(), second_topic.level(), second_topic.position()));
    EXPECT_THAT(assessments, SizeIs(1));
    EXPECT_NO_THROW(assessments = m_database->get_assessments(m_user->id(), subject.id(), third_topic.level(), third_topic.position()));
    EXPECT_THAT(assessments, SizeIs(1));
    std::vector<flashback::Coverage> coverage{};
    EXPECT_NO_THROW(coverage = m_database->get_assessment_coverage(subject.id(), first_topic.position(), first_topic.level()));
    EXPECT_THAT(coverage, SizeIs(1));
    ASSERT_NO_THROW(coverage.at(0).card().id());
    ASSERT_THAT(coverage.at(0).card().id(), Eq(first_card.id()));
    EXPECT_THAT(coverage.at(0).coverage(), Eq(1));
    EXPECT_NO_THROW(coverage = m_database->get_assessment_coverage(subject.id(), second_topic.position(), second_topic.level()));
    EXPECT_THAT(coverage, SizeIs(1));
    ASSERT_NO_THROW(coverage.at(0).card().id());
    ASSERT_THAT(coverage.at(0).card().id(), Eq(second_card.id()));
    EXPECT_THAT(coverage.at(0).coverage(), Eq(1));
    EXPECT_NO_THROW(coverage = m_database->get_assessment_coverage(subject.id(), third_topic.position(), third_topic.level()));
    EXPECT_THAT(coverage, SizeIs(1));
    ASSERT_NO_THROW(coverage.at(0).card().id());
    EXPECT_THAT(coverage.at(0).card().id(), third_card.id());
    EXPECT_THAT(coverage.at(0).coverage(), Eq(1));
    std::map<uint64_t, flashback::Assimilation> assimilation_coverage{};
    EXPECT_NO_THROW(assimilation_coverage = m_database->get_assimilation_coverage(m_user->id(), subject.id(), first_card.id()));
    EXPECT_THAT(assimilation_coverage, SizeIs(1));
    ASSERT_NO_THROW(assimilation_coverage.at(1).assimilated());
    EXPECT_FALSE(assimilation_coverage.at(1).assimilated());
    EXPECT_NO_THROW(assimilation_coverage = m_database->get_assimilation_coverage(m_user->id(), subject.id(), second_card.id()));
    EXPECT_THAT(assimilation_coverage, SizeIs(1));
    ASSERT_NO_THROW(assimilation_coverage.at(2).assimilated());
    EXPECT_FALSE(assimilation_coverage.at(2).assimilated());
    EXPECT_NO_THROW(assimilation_coverage = m_database->get_assimilation_coverage(m_user->id(), subject.id(), third_card.id()));
    EXPECT_THAT(assimilation_coverage, SizeIs(1));
    ASSERT_NO_THROW(assimilation_coverage.at(3).assimilated());
    EXPECT_FALSE(assimilation_coverage.at(3).assimilated());
    bool assimilated{};
    EXPECT_NO_THROW(assimilated = m_database->is_assimilated(m_user->id(), subject.id(), first_topic.level(), first_topic.position()));
    EXPECT_FALSE(assimilated);
    EXPECT_NO_THROW(assimilated = m_database->is_assimilated(m_user->id(), subject.id(), second_topic.level(), second_topic.position()));
    EXPECT_FALSE(assimilated);
    EXPECT_NO_THROW(assimilated = m_database->is_assimilated(m_user->id(), subject.id(), third_topic.level(), third_topic.position()));
    EXPECT_FALSE(assimilated);
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), first_card.id(), 20, mode));
    EXPECT_NO_THROW(throw_back_progress(m_user->id(), first_card.id(), std::chrono::days(4)));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), first_card.id(), 20, mode));
    EXPECT_NO_THROW(throw_back_progress(m_user->id(), first_card.id(), std::chrono::days(3)));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), first_card.id(), 20, mode));
    EXPECT_NO_THROW(throw_back_progress(m_user->id(), first_card.id(), std::chrono::days(2)));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), first_card.id(), 20, mode));
    EXPECT_NO_THROW(throw_back_progress(m_user->id(), first_card.id(), std::chrono::days(1)));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), second_card.id(), 20, mode));
    EXPECT_NO_THROW(throw_back_progress(m_user->id(), second_card.id(), std::chrono::days(3)));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), second_card.id(), 20, mode));
    EXPECT_NO_THROW(throw_back_progress(m_user->id(), second_card.id(), std::chrono::days(2)));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), second_card.id(), 20, mode));
    EXPECT_NO_THROW(throw_back_progress(m_user->id(), second_card.id(), std::chrono::days(1)));
    ASSERT_NO_THROW(m_database->make_progress(m_user->id(), third_card.id(), 20, mode));
    EXPECT_NO_THROW(throw_back_progress(m_user->id(), third_card.id(), std::chrono::days(1)));
    EXPECT_NO_THROW(assimilated = m_database->is_assimilated(m_user->id(), subject.id(), first_topic.level(), first_topic.position()));
    EXPECT_TRUE(assimilated) << "Card was practiced 3 times in progressive mode";
    EXPECT_NO_THROW(assimilated = m_database->is_assimilated(m_user->id(), subject.id(), second_topic.level(), second_topic.position()));
    EXPECT_FALSE(assimilated) << "Card was practiced once in aggressive mode but 2 times in progressive which should not be enough to assimilate";
    EXPECT_NO_THROW(assimilated = m_database->is_assimilated(m_user->id(), subject.id(), third_topic.level(), third_topic.position()));
    EXPECT_FALSE(assimilated) << "Card was practiced only in agrgressive mode but none in progressive";
}

TEST_F(test_database, get_variations)
{
}

TEST_F(test_database, is_absolute)
{
}
