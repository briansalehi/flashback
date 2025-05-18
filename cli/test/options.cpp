#include <flashback/options.hpp>
#include <flashback/exceptions.hpp>
#include <gtest/gtest.h>
#include <string>
#include <vector>

TEST(Options, EmptyOptions)
{
    ASSERT_NO_THROW(flashback::options{std::vector<std::string>{}});
}

TEST(Options, InvalidOption)
{
    std::vector<std::string> const args{"--non-existing-command"};
    ASSERT_THROW(flashback::options{args}, boost::program_options::unknown_option);
}

TEST(Options, HelpOption)
{
    std::vector<std::string> const args{"--help"};
    ASSERT_THROW(flashback::options{args}, flashback::descriptive_option);
}

TEST(Options, VersionOption)
{
    std::vector<std::string> const args{"--version"};
    std::string const version{PROGRAM_VERSION};
    ASSERT_THROW(flashback::options{args}, flashback::descriptive_option);
}