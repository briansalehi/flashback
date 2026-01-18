#!/usr/bin/env bash

sudo -u postgres psql -c 'create role flashback with login connection limit 3'
sudo -u postgres psql -c 'drop database if exists flashback_test'
sudo -u postgres psql -c 'create database flashback_test with owner flashback connection limit 3'
sudo -u postgres psql -c 'create extension if not exists pg_trgm with schema flashback'
sudo -u postgres psql -d flashback_test -f database/schema.sql
