#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <flashback/basic_config_manager.hpp>

class mock_config_manager: public flashback::basic_config_manager
{
public:
    MOCK_METHOD(std::filesystem::path, base_path, (), (const, override, noexcept));
    MOCK_METHOD(std::filesystem::path, config_path, (), ( const, override, noexcept));
    MOCK_METHOD(void, base_path, (std::filesystem::path), (override));
    MOCK_METHOD(void, load, (), (override));
    MOCK_METHOD(void, store, (std::shared_ptr<flashback::User>), (override));
    MOCK_METHOD(std::shared_ptr<flashback::User>, get_user, (), ( const, override));
    MOCK_METHOD(void, make_base, (), (override));
};
