#!/usr/bin/env bash

# Your user should already be a superuser in the cluster in order to execute the following lines
psql postgres -c 'create role flashback with nologin connection limit 2;'
psql postgres -c 'create database flashback with owner flashback connection limit 2;'
psql flashback -c 'create schema flashback authorization flashback;'
psql flashback -f database/flashback.sql
