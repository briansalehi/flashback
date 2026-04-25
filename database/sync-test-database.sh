#!/usr/bin/env bash

base_dir=""
temp_database="$(mktemp -d)/flashback.sql"

if [ "$USER" != "postgres" ]
then
    echo -e "\e[1;2;33mrun again as postgres user!\e[0m" >&2
fi

if [ $# -eq 1 ] && [ -d "$1" ] && [ -f "$1/flashback.sql" ]
then
    base_dir="$1"
elif [ -f $(dirname "$0")/flashback.sql ]
then
    base_dir="$(dirname "$0")"
elif [ -f /opt/flashback.sql ]
then
    base_dir=/opt
elif [ -f flashback.sql ]
then
    base_dir="."
else
    echo -e "\e[1;2;31mspecify the directory containing flashback.sql\e[0m"
    exit 2
fi

database="$base_dir/flashback.sql"

if ! pg_isready 1>/dev/null
then
    echo -e "\e[1;31mdatabase is not running\e[0m "
    exit 1
fi

echo -ne "\e[1;35mdropping previous test database:\e[0m "
psql -c 'drop database flashback_test' || echo -e "\e[1;31mfailed\e[0m"

echo -ne "\e[1;35mcreating a new test database:\e[0m "
psql -c 'create database flashback_test with owner flashback connection limit 10' || echo -e "\e[1;31mfailed\e[0m"

echo -ne "\e[1;35mloading live data to the test database\e[0m "
psql -d flashback_test -f "$database" -q 1>/dev/null && echo || echo -e "\e[1;31mfailed\e[0m"

echo -ne "\e[1;35mdumping database schema to a temporary file\e[0m "
pg_dump -d flashback_test -f "$temp_database" --schema-only && echo || echo -e "\e[1;31mfailed\e[0m"

echo -ne "\e[1;35mdropping test database with live data:\e[0m "
psql -c 'drop database flashback_test' || echo -e "\e[1;31mfailed\e[0m"

echo -ne "\e[1;35mcreating a new test database:\e[0m "
psql -c 'create database flashback_test with owner flashback connection limit 10' || echo -e "\e[1;31mfailed\e[0m"

echo -ne "\e[1;35mloading database schema into the test database\e[0m "
psql -d flashback_test -f "$temp_database" -q 1>/dev/null && echo || echo -e "\e[1;31mfailed\e[0m"

echo -ne "\e[1;35mchanging the role of user brian:\e[0m "
psql -c 'alter role brian in database flashback_test set role = flashback'|| echo -e "\e[1;31mfailed\e[0m"

echo -ne "\e[1;35mchanging the role of user flashback client:\e[0m "
psql -c 'alter role flashback_client in database flashback_test set role = flashback'|| echo -e "\e[1;31mfailed\e[0m"

echo -ne "\e[1;35mchanging the search path fo user brian:\e[0m "
psql -c 'alter role brian in database flashback_test set search_path = flashback, public'|| echo -e "\e[1;31mfailed\e[0m"

echo -ne "\e[1;35mchanging the search path fo user flashback client:\e[0m "
psql -c 'alter role flashback_client in database flashback_test set search_path = flashback, public' || echo -e "\e[1;31mfailed\e[0m"

echo -ne "\e[1;35mremoving temporary database file\e[0m "
rm -r "$(dirname "$temp_database")" && echo || echo -e "\e[1;31mfailed\e[0m"
