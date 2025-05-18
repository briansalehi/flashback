#include <gtest/gtest.h>
#include <boost/process.hpp>
#include <string>
#include <filesystem>
#include <flashback/database.hpp>

class database_test : public testing::Test
{
protected:
    flashback::database database;
    boost::asio::io_context context;

protected:
    database_test(): database{}
    {
    }
};

TEST_F(database_test, database_exists)
{
    boost::process::ipstream oid_out;
    boost::process::child oid_process{"/usr/bin/psql", "-At", "-U", "flashback", "-d", "flashback", "-c", "select oid from pg_database where datname = 'flashback'", boost::process::std_out > oid_out };
    std::string database_id;
    std::getline(oid_out, database_id);
    oid_process.wait();

    boost::process::ipstream base_out;
    boost::process::child base_process{"/usr/bin/psql", "-At", "-U", "postgres", "-d", "postgres", "-c", "show data_directory", boost::process::std_out > base_out};
    std::string base_path;
    std::getline(base_out, base_path);
    std::filesystem::path database_path{std::filesystem::path{base_path} / "base" / database_id};
    base_process.wait();

    unsigned long oid{};

    ASSERT_FALSE(database_id.empty());
    ASSERT_NO_THROW(oid = std::stoul(database_id));
    EXPECT_GT(oid, 0ul);
    ASSERT_TRUE(std::filesystem::exists(database_path));
}