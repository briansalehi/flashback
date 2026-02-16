#!/usr/bin/env bash

sudo -u postgres pg_ctl start -D /var/lib/pgsql/data -l /var/lib/pgsql/logfile
sudo -u postgres psql -c 'drop database if exists flashback'
sudo -u postgres psql -c 'create role flashback with nologin'
sudo -u postgres psql -c 'create role brian with login connection limit 1'
sudo -u postgres psql -c 'create role flashback_client with login connection limit 3'
sudo -u postgres psql -c 'create database flashback with owner flashback connection limit 3'
sudo -u postgres psql -d flashback -c 'create schema if not exists flashback authorization flashback'
sudo -u postgres psql -d flashback -c 'create extension if not exists citext with schema flashback'
sudo -u postgres psql -d flashback -c 'create extension if not exists pg_trgm with schema flashback'
sudo -u postgres psql -d flashback -c 'revoke all on schema public from public'
sudo -u postgres psql -d flashback -c 'grant usage, create on schema public to flashback_client'
sudo -u postgres psql -d flashback -c 'grant usage, create on schema public to brian'
sudo -u postgres psql -c 'grant flashback to flashback_client'
sudo -u postgres psql -c 'grant flashback to brian'
sudo -u postgres psql -c 'alter role brian in database flashback set role = flashback'
sudo -u postgres psql -c 'alter role flashback_client in database flashback set role = flashback'
sudo -u postgres psql -c 'alter role brian in database flashback set search_path = flashback, public'
sudo -u postgres psql -c 'alter role flashback_client in database flashback set search_path = flashback, public'
sudo -u postgres psql -d flashback -f "$(dirname "$(readlink -f "$0")")"/../database/flashback.sql -q
/usr/local/bin/flashbackd
