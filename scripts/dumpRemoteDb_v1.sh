#!/bin/bash

FILE_NAME="dump_v1.sql"

rm -f $FILE_NAME

pg_dump $POSTGRES_HOST >> $FILE_NAME