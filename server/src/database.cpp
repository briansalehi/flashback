#include <format>
#include <iostream>
#include <cstdlib>
#include <flashback/database.hpp>

using namespace flashback;

database::database(std::string address, std::string port)
    : m_connection{nullptr}
{
    try
    {
        m_connection = std::make_unique<pqxx::connection>(std::format("postgres://flashback@{}:{}/flashback", address, port));
    }
    catch (pqxx::broken_connection const& exp)
    {
        std::cerr << exp.what() << std::endl;
        std::exit(1);
    }
    catch (pqxx::disk_full const& exp)
    {
        std::cerr << exp.what() << std::endl;
        std::exit(2);
    }
    catch (pqxx::argument_error const& exp)
    {
        std::cerr << exp.what() << std::endl;
        std::exit(3);
    }
    catch (pqxx::data_exception const& exp)
    {
        std::cerr << exp.what() << std::endl;
        std::exit(4);
    }
}

pqxx::result database::query(std::string_view const statement) const
{
    pqxx::work work{*m_connection};
    pqxx::result result{work.exec(statement)};
    work.commit();
    return result;
}
