#include <memory>
#include <ranges>
#include <vector>
#include <exception>
#include <algorithm>
#include <filesystem>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <pqxx/pqxx>
#include <flashback/database.hpp>
#include <flashback/exception.hpp>

class test_database: public testing::Test
{
protected:
    void SetUp() override
    {
        m_connection = std::make_unique<pqxx::connection>("postgres://flashback@localhost:5432/flashback_test");
        m_database = std::make_unique<flashback::database>("flashback_test");
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
        EXPECT_GT(user_id, 0);

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

    std::unique_ptr<pqxx::connection> m_connection{nullptr};
    std::shared_ptr<flashback::database> m_database{nullptr};
    std::unique_ptr<flashback::User> m_user{nullptr};
};

TEST_F(test_database, CreateDuplicateUser)
{
    uint64_t user_id{};
    EXPECT_NO_THROW(user_id = m_database->create_user(m_user->name(), m_user->email(), m_user->hash()));
    ASSERT_EQ(user_id, 0);
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

    ASSERT_EQ(user, nullptr);

    std::string unknown_device{R"(33333333-3333-3333-3333-333333333333)"};
    user = m_database->get_user(m_user->token(), m_user->device());

    ASSERT_EQ(user, nullptr);
    ASSERT_TRUE(m_database->create_session(m_user->id(), m_user->token(), m_user->device()));

    user = m_database->get_user(m_user->token(), m_user->device());

    ASSERT_NE(user, nullptr);
    EXPECT_GT(user->id(), 0);
    EXPECT_EQ(user->name(), m_user->name());
    EXPECT_EQ(user->email(), m_user->email());
    EXPECT_EQ(user->hash(), m_user->hash());
    EXPECT_EQ(user->token(), m_user->token());
    EXPECT_EQ(user->device(), m_user->device());
    EXPECT_TRUE(user->password().empty());
    EXPECT_FALSE(user->verified());
}

TEST_F(test_database, GetUserWithEmail)
{
    std::string non_existing_email{"non_existing_user@flashback.eu.com"};
    std::unique_ptr<flashback::User> user{m_database->get_user(non_existing_email)};

    ASSERT_EQ(user, nullptr);

    user = m_database->get_user(m_user->email());

    ASSERT_NE(user, nullptr);
    EXPECT_GT(user->id(), 0);
    EXPECT_EQ(user->name(), m_user->name());
    EXPECT_EQ(user->email(), m_user->email());
    EXPECT_EQ(user->hash(), m_user->hash());
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
    EXPECT_NE(user, nullptr);

    user = m_database->get_user(token1, device1);
    ASSERT_EQ(user, nullptr);

    user = m_database->get_user(token2, device2);
    EXPECT_EQ(user, nullptr);
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
    ASSERT_EQ(user, nullptr);

    user = m_database->get_user(m_user->token(), m_user->device());
    EXPECT_NE(user, nullptr);
}

TEST_F(test_database, ResetPassword)
{
    std::unique_ptr<flashback::User> user{nullptr};
    std::string hash{R"($argon2id$v=19$m=262144,t=3,p=1$faiJerPACLb2TEdTbGv8AQ$M0j9j6ojyIjD9yZ4+lAANR/WAiWpImUcEcUhCL3u9gc)"};

    ASSERT_TRUE(m_database->create_session(m_user->id(), m_user->token(), m_user->device()));

    user = m_database->get_user(m_user->token(), m_user->device());

    ASSERT_NE(user, nullptr);
    EXPECT_EQ(user->hash(), m_user->hash());

    m_database->reset_password(m_user->id(), hash);

    user = m_database->get_user(m_user->token(), m_user->device());
    ASSERT_NE(user, nullptr);
    EXPECT_EQ(user->hash(), hash);
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
    EXPECT_GT(primary_roadmap.id(), 0);

    EXPECT_NO_THROW(secondary_roadmap = m_database->create_roadmap(m_user->id(), secondary_roadmap.name()));
    EXPECT_GT(secondary_roadmap.id(), 0);

    EXPECT_THROW(failed_roadmap = m_database->create_roadmap(m_user->id(), primary_roadmap.name()), pqxx::unique_violation);

    EXPECT_NO_THROW(quoted_roadmap = m_database->create_roadmap(m_user->id(), quoted_roadmap.name()));
    EXPECT_GT(quoted_roadmap.id(), 0);

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
    ASSERT_GT(primary_roadmap.id(), 0);
    primary_roadmap.set_id(primary_roadmap.id());

    ASSERT_NO_THROW(secondary_roadmap = m_database->create_roadmap(m_user->id(), secondary_roadmap.name()));
    ASSERT_GT(secondary_roadmap.id(), 0);

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
    ASSERT_GT(roadmap.id(), 0);

    EXPECT_NO_THROW(m_database->rename_roadmap(roadmap.id(), modified_name));
    ASSERT_NO_THROW(roadmaps = m_database->get_roadmaps(m_user->id()));
    EXPECT_EQ(roadmaps.size(), 1);
    ASSERT_NO_THROW(roadmap = roadmaps.at(0));
    EXPECT_EQ(roadmaps.at(0).name(), modified_name);

    EXPECT_NO_THROW(m_database->rename_roadmap(roadmap.id(), name_with_quotes));
    ASSERT_NO_THROW(roadmaps = m_database->get_roadmaps(m_user->id()));
    EXPECT_EQ(roadmaps.size(), 1);
    ASSERT_NO_THROW(roadmap = roadmaps.at(0));
    EXPECT_EQ(roadmap.name(), name_with_quotes);

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
    ASSERT_GT(primary_roadmap.id(), 0);
    ASSERT_GT(secondary_roadmap.id(), 0);

    EXPECT_NO_THROW(m_database->remove_roadmap(primary_roadmap.id()));
    ASSERT_THAT(m_database->get_roadmaps(m_user->id()), testing::SizeIs(1)) << "Of two existing roadmaps, one should remain";
    EXPECT_NO_THROW(primary_roadmap = m_database->get_roadmaps(m_user->id()).at(0));
    EXPECT_EQ(primary_roadmap.name(), secondary_roadmap.name());

    EXPECT_NO_THROW(m_database->remove_roadmap(primary_roadmap.id())) << "Deleting non-existing roadmap should not throw an exception";
}

TEST_F(test_database, SearchRoadmaps)
{
    std::vector<std::string> const names{"Cloud Infrastructure",
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
                                         "Growth Strategy"};
    std::vector<flashback::Roadmap> search_results;
    search_results.reserve(names.size());

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
    EXPECT_EQ(subject.name(), subject_name);
    EXPECT_GT(subject.id(), 0);

    EXPECT_THROW(subject = m_database->create_subject(subject_name), pqxx::unique_violation) << "Subjects must be unique";

    subject_name = "Linus' Operating System";
    EXPECT_NO_THROW(subject = m_database->create_subject(subject_name)) << "Subjects with quotes in their name should not be a problem";
    EXPECT_EQ(subject.name(), subject_name);
    EXPECT_GT(subject.id(), 0);

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
    ASSERT_GT(subject.id(), 0);

    ASSERT_NO_THROW(subject = m_database->create_subject(subject_name)) << "Subject sample 2 should be created before performing search";
    ASSERT_EQ(subject.name(), subject_name);
    ASSERT_GT(subject.id(), 0);

    EXPECT_NO_THROW(matches = m_database->search_subjects(name_pattern));
    EXPECT_EQ(matches.size(), 1) << "Only one of the two existing objects should be similar";
    EXPECT_NO_THROW(matches.at(1)) << "The first and only match should be in the first position";
    EXPECT_EQ(matches.at(1).id(), subject.id());
    EXPECT_EQ(matches.at(1).name(), subject.name());

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
    ASSERT_EQ(subject.name(), irrelevant_name);
    ASSERT_GT(subject.id(), 0);

    ASSERT_NO_THROW(subject = m_database->create_subject(initial_name));
    ASSERT_EQ(subject.name(), initial_name);
    ASSERT_GT(subject.id(), 0);

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
    EXPECT_EQ(matches.at(expected_position).name(), modified_name);

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
    ASSERT_GT(returning_roadmap.id(), 0);

    for (auto const& name: subject_names)
    {
        flashback::Subject returning_subject;
        flashback::Subject expected_subject;
        flashback::Milestone milestone;
        expected_subject.set_name(name);
        ASSERT_NO_THROW(returning_subject = m_database->create_subject(expected_subject.name()));
        ASSERT_EQ(returning_subject.name(), expected_subject.name());
        ASSERT_GT(returning_subject.id(), 0);
        EXPECT_NO_THROW(milestone = m_database->add_milestone(returning_subject.id(), flashback::expertise_level::surface, returning_roadmap.id()));
        EXPECT_GT(milestone.id(), 0);
        EXPECT_GT(milestone.position(), 0);
        EXPECT_EQ(milestone.level(), flashback::expertise_level::surface);
    }
}

TEST_F(test_database, AddMilestoneWithPosition)
{
    std::vector<std::string> const subject_names{"GitHub Actions", "Valgrind", "NeoVim", "OpenSSL"};
    flashback::Roadmap returning_roadmap{};
    flashback::Roadmap expected_roadmap{};
    expected_roadmap.set_name("Open Source Expert");

    ASSERT_NO_THROW(returning_roadmap = m_database->create_roadmap(m_user->id(), expected_roadmap.name()));
    ASSERT_GT(returning_roadmap.id(), 0);

    uint64_t position{};

    for (auto const& name: subject_names)
    {
        ++position;
        flashback::Subject returning_subject;
        flashback::Subject expected_subject;
        flashback::Milestone milestone;
        expected_subject.set_name(name);
        ASSERT_NO_THROW(returning_subject = m_database->create_subject(expected_subject.name()));
        ASSERT_EQ(returning_subject.name(), expected_subject.name());
        ASSERT_GT(returning_subject.id(), 0);
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
    ASSERT_GT(returning_roadmap.id(), 0);

    for (auto const& name: subject_names)
    {
        flashback::Subject returning_subject;
        flashback::Subject expected_subject;
        flashback::Milestone milestone;
        expected_subject.set_name(name);
        ASSERT_NO_THROW(returning_subject = m_database->create_subject(expected_subject.name()));
        ASSERT_EQ(returning_subject.name(), expected_subject.name());
        ASSERT_GT(returning_subject.id(), 0);
        EXPECT_NO_THROW(milestone = m_database->add_milestone(returning_subject.id(), flashback::expertise_level::surface, returning_roadmap.id()));
        EXPECT_EQ(milestone.id(), returning_subject.id());
        EXPECT_EQ(milestone.level(), flashback::expertise_level::surface);
    }

    EXPECT_NO_THROW(milestones = m_database->get_milestones(returning_roadmap.id()));
    ASSERT_EQ(milestones.size(), subject_names.size());

    for (auto const& milestone: milestones)
    {
        EXPECT_GT(milestone.position(), 0);
        auto const& iter = std::ranges::find_if(subject_names, [&milestone](auto const& name) { return name == milestone.name(); });
        EXPECT_NE(iter, subject_names.cend());
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
    ASSERT_GT(roadmap.id(), 0);
    ASSERT_NO_THROW(dependent_subject = m_database->create_subject(dependent_subject.name()));
    ASSERT_GT(dependent_subject.id(), 0);
    ASSERT_NO_THROW(required_subject = m_database->create_subject(required_subject.name()));
    ASSERT_GT(dependent_subject.id(), 0);
    ASSERT_NO_THROW(dependent_milestone = m_database->add_milestone(dependent_subject.id(), dependent_milestone.level(), roadmap.id()));
    ASSERT_EQ(dependent_milestone.id(), dependent_subject.id());
    ASSERT_NO_THROW(required_milestone = m_database->add_milestone(required_subject.id(), required_milestone.level(), roadmap.id()));
    ASSERT_EQ(required_milestone.id(), required_subject.id());
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
    ASSERT_GT(roadmap.id(), 0);
    ASSERT_NO_THROW(dependent_subject = m_database->create_subject(dependent_subject.name()));
    ASSERT_GT(dependent_subject.id(), 0);
    ASSERT_NO_THROW(required_subject = m_database->create_subject(required_subject.name()));
    ASSERT_GT(required_subject.id(), 0);
    ASSERT_NO_THROW(dependent_milestone = m_database->add_milestone(dependent_subject.id(), dependent_milestone.level(), roadmap.id()));
    ASSERT_EQ(dependent_milestone.id(), dependent_subject.id());
    ASSERT_NO_THROW(required_milestone = m_database->add_milestone(required_subject.id(), required_milestone.level(), roadmap.id()));
    ASSERT_EQ(required_milestone.id(), required_subject.id());
    ASSERT_NO_THROW(m_database->add_requirement(roadmap.id(), dependent_milestone, required_milestone));
    EXPECT_NO_THROW(requirements = m_database->get_requirements(roadmap.id(), dependent_milestone.id(), dependent_milestone.level()));
    ASSERT_EQ(requirements.size(), 1);
    ASSERT_NO_THROW(requirements.at(0));
    EXPECT_EQ(requirements.at(0).id(), required_milestone.id());
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
    ASSERT_GT(user_id, 0);
    ASSERT_TRUE(m_database->create_session(user_id, std::move(token), std::move(device)));
    ASSERT_NO_THROW(roadmap = m_database->create_roadmap(m_user->id(), roadmap.name()));
    ASSERT_GT(roadmap.id(), 0);
    EXPECT_NO_THROW(cloned_roadmap = m_database->clone_roadmap(user_id, roadmap.id())) << "Roadmap should be cloned for the new user";
    ASSERT_NO_THROW(roadmaps = m_database->get_roadmaps(user_id));
    EXPECT_EQ(roadmaps.size(), 1) << "New user should have one cloned roadmap";
    EXPECT_NO_THROW(roadmaps.at(0));
    EXPECT_GT(roadmaps.at(0).id(), 0);
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
    EXPECT_EQ(milestone_pos1.position(), 1);
    EXPECT_EQ(milestone_pos2.position(), 2);
    EXPECT_EQ(milestone_pos3.position(), 3);
    ASSERT_NO_THROW(milestones = m_database->get_milestones(roadmap.id()));
    ASSERT_THAT(milestones, testing::SizeIs(3));
    ASSERT_NO_THROW(milestones.at(2));
    EXPECT_EQ(milestones.at(0).id(), milestone_pos1.id()) << "Milestone 1 should be in position 1 before reordering";
    EXPECT_EQ(milestones.at(0).position(), 1) << "Vector should have the same order as milestones";
    EXPECT_EQ(milestones.at(1).id(), milestone_pos2.id()) << "Milestone 2 should be in position 2 before reordering";
    EXPECT_EQ(milestones.at(1).position(), 2) << "Vector should have the same order as milestones";
    EXPECT_EQ(milestones.at(2).id(), milestone_pos3.id()) << "Milestone 3 should be in position 3 before reordering";
    EXPECT_EQ(milestones.at(2).position(), 3) << "Vector should have the same order as milestones";
    EXPECT_NO_THROW(m_database->reorder_milestone(roadmap.id(), 1, 3));
    ASSERT_NO_THROW(milestones = m_database->get_milestones(roadmap.id()));
    ASSERT_THAT(milestones, testing::SizeIs(3));
    ASSERT_NO_THROW(milestones.at(2));
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
    ASSERT_GT(roadmap.id(), 0);

    for (std::string const& name: {"Calculus", "Linear Algebra", "Graph Theory"})
    {
        flashback::Subject subject{};
        flashback::Milestone milestone{};
        ASSERT_NO_THROW(subject = m_database->create_subject(name));
        ASSERT_GT(subject.id(), 0);
        ASSERT_NO_THROW(milestone = m_database->add_milestone(subject.id(), flashback::expertise_level::depth, roadmap.id()));
        ASSERT_EQ(milestone.id(), subject.id());
    }

    ASSERT_NO_THROW(milestones = m_database->get_milestones(roadmap.id()));
    ASSERT_THAT(milestones, SizeIs(3));
    ASSERT_NO_THROW(milestones.at(2));
    EXPECT_NO_THROW(m_database->remove_milestone(roadmap.id(), milestones.at(2).id()));
    ASSERT_NO_THROW(milestones = m_database->get_milestones(roadmap.id()));
    ASSERT_THAT(milestones, SizeIs(2));
    ASSERT_NO_THROW(milestones.at(1));
    EXPECT_GT(milestones.at(0).id(), 0);
    EXPECT_GT(milestones.at(1).id(), 0);
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
    ASSERT_GT(roadmap.id(), 0);

    subject.set_name("Calculus");
    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_GT(subject.id(), 0);
    ASSERT_NO_THROW(milestone = m_database->add_milestone(subject.id(), flashback::expertise_level::depth, roadmap.id()));
    ASSERT_EQ(milestone.id(), subject.id());
    ASSERT_NO_THROW(milestones = m_database->get_milestones(roadmap.id()));
    ASSERT_THAT(milestones, SizeIs(1));
    ASSERT_NO_THROW(modified_milestone = milestones.at(0));
    ASSERT_EQ(modified_milestone.id(), subject.id());
    EXPECT_EQ(modified_milestone.level(), flashback::expertise_level::depth);
    EXPECT_NO_THROW(m_database->change_milestone_level(roadmap.id(), milestone.id(), flashback::expertise_level::surface));
    ASSERT_NO_THROW(milestones = m_database->get_milestones(roadmap.id()));
    ASSERT_THAT(milestones, SizeIs(1));
    ASSERT_NO_THROW(modified_milestone = milestones.at(0));
    ASSERT_EQ(modified_milestone.id(), subject.id());
    EXPECT_EQ(modified_milestone.level(), flashback::expertise_level::surface);
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
    ASSERT_GT(subject.id(), 0);
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
    EXPECT_GT(resource.id(), 0);
    EXPECT_EQ(resource.name(), name);
    EXPECT_EQ(resource.type(), type);
    EXPECT_EQ(resource.pattern(), pattern);
    EXPECT_EQ(resource.link(), link);
    EXPECT_EQ(resource.production(), production);
    EXPECT_EQ(resource.expiration(), expiration);

    resource.clear_id();
    resource.clear_name();
    EXPECT_NO_THROW(resource = m_database->create_resource(resource));
    EXPECT_EQ(resource.id(), 0) << "Resource with empty name should be created";
    EXPECT_TRUE(resource.name().empty());
    EXPECT_EQ(resource.type(), type);
    EXPECT_EQ(resource.pattern(), pattern);
    EXPECT_EQ(resource.link(), link);
    EXPECT_EQ(resource.production(), production);
    EXPECT_EQ(resource.expiration(), expiration);
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
    ASSERT_GT(resource.id(), 0);
    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_GT(subject.id(), 0);
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
    ASSERT_GT(resource.id(), 0);
    ASSERT_NO_THROW(secondary_resource = m_database->create_resource(secondary_resource));
    ASSERT_GT(secondary_resource.id(), 0);
    ASSERT_NE(resource.id(), secondary_resource.id());
    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_GT(subject.id(), 0);
    EXPECT_NO_THROW(resources = m_database->get_resources(subject.id() + 1));
    EXPECT_THAT(resources, SizeIs(0)) << "Invalid subject should not have resources";
    ASSERT_NO_THROW(m_database->add_resource_to_subject(resource.id(), subject.id()));
    ASSERT_NO_THROW(m_database->add_resource_to_subject(secondary_resource.id(), subject.id()));
    EXPECT_NO_THROW(resources = m_database->get_resources(subject.id()));
    EXPECT_THAT(resources, SizeIs(2));
    ASSERT_NO_THROW(resources.at(0));
    EXPECT_EQ(resources.at(0).id(), resource.id());
    EXPECT_EQ(resources.at(0).name(), resource.name());
    EXPECT_EQ(resources.at(0).type(), resource.type());
    EXPECT_EQ(resources.at(0).pattern(), resource.pattern());
    EXPECT_EQ(resources.at(0).link(), resource.link());
    EXPECT_EQ(resources.at(0).production(), resource.production());
    EXPECT_EQ(resources.at(0).expiration(), resource.expiration());
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
    ASSERT_GT(resource.id(), 0);
    ASSERT_NO_THROW(secondary_resource = m_database->create_resource(secondary_resource));
    ASSERT_GT(secondary_resource.id(), 0);
    ASSERT_NE(resource.id(), secondary_resource.id());
    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_GT(subject.id(), 0);
    ASSERT_NO_THROW(m_database->add_resource_to_subject(resource.id(), subject.id()));
    EXPECT_NO_THROW(resources = m_database->get_resources(subject.id()));
    EXPECT_THAT(resources, SizeIs(1));
    resources.clear();
    ASSERT_NO_THROW(m_database->add_resource_to_subject(secondary_resource.id(), subject.id()));
    EXPECT_NO_THROW(resources = m_database->get_resources(subject.id()));
    EXPECT_THAT(resources, SizeIs(2));
    EXPECT_NO_THROW(m_database->drop_resource_from_subject(resource.id(), subject.id()));
    EXPECT_NO_THROW(resources = m_database->get_resources(subject.id()));
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
    ASSERT_GT(subject.id(), 0);

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
        ASSERT_GT(resource.id(), 0);
        ASSERT_NO_THROW(m_database->add_resource_to_subject(resource.id(), subject.id()));
        resources.push_back(std::move(resource));
    }

    EXPECT_NO_THROW(resources = m_database->get_resources(subject.id()));
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
    ASSERT_GT(resource.id(), 0);
    ASSERT_NO_THROW(subject = m_database->create_subject(subject.name()));
    ASSERT_GT(subject.id(), 0);
    ASSERT_NO_THROW(m_database->add_resource_to_subject(resource.id(), subject.id()));
    ASSERT_NO_THROW(resources = m_database->get_resources(subject.id()));
    ASSERT_THAT(resources, SizeIs(1));
    ASSERT_NO_THROW(resources.at(0).id());
    ASSERT_EQ(resources.at(0).id(), resource.id());
    EXPECT_EQ(resources.at(0).link(), resource.link());
    EXPECT_NE(resources.at(0).link(), modified_link);
    EXPECT_NO_THROW(m_database->edit_resource_link(resource.id(), modified_link));
    ASSERT_NO_THROW(resources = m_database->get_resources(subject.id()));
    ASSERT_THAT(resources, SizeIs(1));
    ASSERT_NO_THROW(resources.at(0).id());
    ASSERT_EQ(resources.at(0).id(), resource.id());
    EXPECT_NE(resources.at(0).link(), resource.link());
    EXPECT_EQ(resources.at(0).link(), modified_link);
}
