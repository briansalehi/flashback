#include <flashback/database.hpp>
#include <format>

using namespace flashback;

database::database(std::string address, std::string port)
    : connection{std::format("host={} port={} dbname=flashback user=flashback", address, port)}
{
}