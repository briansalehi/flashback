#!/usr/bin/env bash

psql -U postgres -c 'create database flashback_test with owner flashback connection limit 3'
psql -U flashback -d flashback_test -f schema.sql
