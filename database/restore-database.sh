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

psql -c 'drop database if exists flashback'
psql -c 'create role flashback with nologin'
psql -c 'create role brian with login connection limit 1'
psql -c 'create role flashback_client with login connection limit 10'
psql -c 'create database flashback with owner flashback connection limit 3'
psql -d flashback -c 'create schema if not exists flashback authorization flashback'
psql -d flashback -c 'create extension if not exists citext with schema flashback'
psql -d flashback -c 'create extension if not exists pg_trgm with schema flashback'
psql -d flashback -c 'revoke all on schema public from public'
psql -d flashback -c 'grant usage, create on schema public to flashback_client'
psql -d flashback -c 'grant usage, create on schema public to brian'
psql -c 'grant flashback to flashback_client'
psql -c 'grant flashback to brian'
psql -c 'alter role brian in database flashback set role = flashback'
psql -c 'alter role flashback_client in database flashback set role = flashback'
psql -c 'alter role brian in database flashback set search_path = flashback, public'
psql -c 'alter role flashback_client in database flashback set search_path = flashback, public'
psql -d flashback -f "$base_dir/flashback.sql" -q
