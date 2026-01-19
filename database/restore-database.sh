#!/usr/bin/env bash

sudo -u postgres psql -c 'drop database if exists flashback'
sudo -u postgres psql -c 'create schema if not exists flashback authorization flashback'
sudo -u postgres psql -c 'create database flashback with owner flashback connection limit 3'
sudo -u postgres psql -c 'create extension if not exists pg_trgm with schema flashback'
sudo -u postgres psql -c 'alter role brian in database flashback set role = flashback'
sudo -u postgres psql -c 'alter role flashback_client in database flashback set role = flashback'
sudo -u postgres psql -c 'alter role brian in database flashback set search_path = flashback, public'
sudo -u postgres psql -c 'alter role flashback_client in database flashback set search_path = flashback, public'
psql -d flashback -f "$(dirname "$(readlink -f "$0")")"/flashback.sql -q
