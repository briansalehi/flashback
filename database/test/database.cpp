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
