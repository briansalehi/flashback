#!/usr/bin/env bash

base_dir=""

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

if ! pg_isready
then
    pg_ctl start -D /var/lib/pgsql/data -l /var/lib/pgsql/logfile
fi

psql -c 'drop database flashback_test'
psql -c 'create database flashback_test with owner flashback connection limit 10'
psql -c 'alter role brian in database flashback_test set role = flashback'
psql -c 'alter role flashback_client in database flashback_test set role = flashback'
psql -c 'alter role brian in database flashback_test set search_path = flashback, public'
psql -c 'alter role flashback_client in database flashback_test set search_path = flashback, public'
psql -d flashback_test -f "$base_dir/schema.sql" -q
