#!/usr/bin/env bash

working_directory="$(dirname "$(readlink -f "$0")")"
pg_dump -U brian -h localhost -d flashback -f "$working_directory"/flashback.sql
pg_dump -U brian -h localhost -d flashback -f "$working_directory"/schema.sql --schema-only
