#!/usr/bin/env bash

sudo -u postgres psql -c 'drop database if exists flashback_test'
sudo -u postgres psql -c 'create role flashback with nologin connection limit 3'
sudo -u postgres psql -c 'create role flashback_client with login connection limit 3'
sudo -u postgres psql -c 'create schema if not exists flashback authorization flashback'
sudo -u postgres psql -c 'grant flashback to flashback_client'
sudo -u postgres psql -c 'create database flashback_test with owner flashback connection limit 3'
sudo -u postgres psql -c 'create extension if not exists pg_trgm with schema flashback'
sudo -u postgres psql -d flashback_test -f database/schema.sql -q
sudo -u postgres psql -c 'revoke all on schema public from public'
sudo -u postgres psql -c 'grant usage, create on schema public to flashback_client'
sudo -u postgres psql -c 'alter role flashback_client in database flashback_test set search_path = flashback, public'
