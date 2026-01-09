#include <memory>
#include <vector>
#include <utility>
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
    uint64_t primary_id{};
    uint64_t secondary_id{};
    uint64_t quoted_id{};
    std::string primary_name{"Social Anxiety Expert"};
    std::string secondary_name{"Pointless Speech Specialist"};
    std::string name_with_quotes{"O'Reilly Technical Editor"};

    EXPECT_NO_THROW(primary_id = m_database->create_roadmap(primary_name));
    EXPECT_GT(primary_id, 0);

    EXPECT_NO_THROW(secondary_id = m_database->create_roadmap(secondary_name));
    EXPECT_GT(secondary_id, 0);

    EXPECT_THROW(m_database->create_roadmap(primary_name), pqxx::unique_violation);

    EXPECT_NO_THROW(quoted_id = m_database->create_roadmap(name_with_quotes));
    EXPECT_GT(quoted_id, 0);

    EXPECT_THROW(m_database->create_roadmap(""), flashback::client_exception);
}

TEST_F(test_database, AssignRoadmapToUser)
{
    uint64_t primary_id{};
    uint64_t secondary_id{};
    uint64_t non_existing_id{1000};
    std::string primary_name{"Social Anxiety Expert"};
    std::string secondary_name{"Pointless Speech Specialist"};

    ASSERT_NO_THROW(primary_id = m_database->create_roadmap(primary_name));
    ASSERT_GT(primary_id, 0);

    ASSERT_NO_THROW(secondary_id = m_database->create_roadmap(secondary_name));
    ASSERT_GT(secondary_id, 0);

    EXPECT_NO_THROW(m_database->assign_roadmap(m_user->id(), primary_id));
    EXPECT_NO_THROW(m_database->assign_roadmap(m_user->id(), secondary_id));
    EXPECT_THROW(m_database->assign_roadmap(m_user->id(), non_existing_id), pqxx::foreign_key_violation);
}

TEST_F(test_database, GetRoadmaps)
{
    uint64_t primary_id{};
    uint64_t secondary_id{};
    std::string primary_name{"Social Anxiety Expert"};
    std::string secondary_name{"Pointless Speech Specialist"};
    flashback::Roadmap primary_roadmap{};
    flashback::Roadmap secondary_roadmap{};
    std::vector<flashback::Roadmap> roadmaps;

    primary_roadmap.set_name(primary_name);

    EXPECT_NO_THROW(roadmaps = m_database->get_roadmaps(m_user->id()));
    EXPECT_THAT(roadmaps, testing::IsEmpty()) << "No roadmap was created so far, container should be empty";

    ASSERT_NO_THROW(primary_id = m_database->create_roadmap(primary_name));
    ASSERT_GT(primary_id, 0);
    primary_roadmap.set_id(primary_id);

    EXPECT_NO_THROW(roadmaps = m_database->get_roadmaps(m_user->id()));
    EXPECT_THAT(roadmaps, testing::IsEmpty()) << "A roadmap was created but is not assigned to the user yet, container should be empty";

    ASSERT_NO_THROW(secondary_id = m_database->create_roadmap(secondary_name));
    ASSERT_GT(secondary_id, 0);
    secondary_roadmap.set_id(secondary_id);

    EXPECT_NO_THROW(roadmaps = m_database->get_roadmaps(m_user->id()));
    EXPECT_THAT(roadmaps, testing::IsEmpty()) << "Regardless of how many roadmaps exist, only the assigned roadmaps should return which is none so far";

    EXPECT_NO_THROW(m_database->assign_roadmap(m_user->id(), primary_id));

    roadmaps = m_database->get_roadmaps(m_user->id());
    EXPECT_THAT(roadmaps, testing::SizeIs(1)) << "One of the two existing roadmaps assigned to the user, so container should hold one roadmap";

    EXPECT_NO_THROW(m_database->assign_roadmap(m_user->id(), secondary_id));

    roadmaps = m_database->get_roadmaps(m_user->id());
    EXPECT_THAT(roadmaps, testing::SizeIs(2)) << "Both of the existing roadmaps was assigned to user, so container should contain both";
}

TEST_F(test_database, RenameRoadmap)
{
    uint64_t roadmap_id{};
    std::string roadmap_name{"Over Engineering Expert"};
    std::string modified_name{"Task Procrastination Engineer"};
    std::string name_with_quotes{"Underestimated Tasks' Deadline Scheduler"};
    std::vector<flashback::Roadmap> roadmaps{};
    flashback::Roadmap roadmap{};

    ASSERT_NO_THROW(roadmap_id = m_database->create_roadmap(roadmap_name));
    ASSERT_GT(roadmap_id, 0);
    ASSERT_NO_THROW(m_database->assign_roadmap(m_user->id(), roadmap_id));

    EXPECT_NO_THROW(m_database->rename_roadmap(roadmap_id, modified_name));
    ASSERT_NO_THROW(roadmaps = m_database->get_roadmaps(m_user->id()));
    EXPECT_EQ(roadmaps.size(), 1);
    ASSERT_NO_THROW(roadmap = roadmaps.at(0));
    EXPECT_EQ(roadmaps.at(0).name(), modified_name);

    EXPECT_NO_THROW(m_database->rename_roadmap(roadmap_id, name_with_quotes));
    ASSERT_NO_THROW(roadmaps = m_database->get_roadmaps(m_user->id()));
    EXPECT_EQ(roadmaps.size(), 1);
    ASSERT_NO_THROW(roadmap = roadmaps.at(0));
    EXPECT_EQ(roadmap.name(), name_with_quotes);

    EXPECT_THROW(m_database->rename_roadmap(roadmap_id, ""), flashback::client_exception);
}

TEST_F(test_database, RemoveRoadmap)
{
    std::string primary_name{"Useless Meetings Creational Engineer"};
    std::string secondary_name{"Variable Naming Specialist"};
    uint64_t primary_id{};
    uint64_t secondary_id{};
    flashback::Roadmap roadmap{};

    ASSERT_NO_THROW(primary_id = m_database->create_roadmap(primary_name));
    ASSERT_NO_THROW(secondary_id = m_database->create_roadmap(secondary_name));
    ASSERT_GT(primary_id, 0);
    ASSERT_GT(secondary_id, 0);
    ASSERT_NO_THROW(m_database->assign_roadmap(m_user->id(), primary_id));
    ASSERT_NO_THROW(m_database->assign_roadmap(m_user->id(), secondary_id));

    EXPECT_NO_THROW(m_database->remove_roadmap(primary_id));
    ASSERT_THAT(m_database->get_roadmaps(m_user->id()), testing::SizeIs(1)) << "Of two existing roadmaps, one should remain";
    EXPECT_NO_THROW(roadmap = m_database->get_roadmaps(m_user->id()).at(0));
    EXPECT_EQ(roadmap.name(), secondary_name);

    EXPECT_NO_THROW(m_database->remove_roadmap(primary_id)) << "Deleting non-existing roadmap should not throw an exception";
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
        uint64_t roadmap_id{};
        ASSERT_NO_THROW(roadmap_id = m_database->create_roadmap(roadmap_name));
        ASSERT_NO_THROW(m_database->assign_roadmap(m_user->id(), roadmap_id));
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
    uint64_t const expected_position{1};
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
