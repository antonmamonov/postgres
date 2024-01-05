#!/bin/bash

FILE_NAME="dump_v1.sql"

# restore the dump to postgres
psql $POSTGRES_HOST < $FILE_NAME