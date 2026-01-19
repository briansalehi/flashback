#!/usr/bin/env bash

sudo -u postgres psql -c 'drop database flashback_test'
sudo -u postgres psql -c 'create database flashback_test with owner flashback connection limit 3'
sudo -u postgres psql -c 'alter role flashback_client in database flashback_test set role = flashback'
sudo -u postgres psql -c 'alter role flashback_client in database flashback_test set search_path = flashback, public'
psql -d flashback_test -f "$(dirname "$(readlink -f "$0")")"/schema.sql -q
