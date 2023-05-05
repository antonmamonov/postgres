#!/bin/bash

echo "[$(date)] Starting Anton PostgreSQL"

# check if the data directory has postgresql.conf
if [ ! -f $PGDATA/postgresql.conf ]; then
    echo "Data directory is empty"
    echo "Initializing database"
    initdb -E UTF8 
fi

postgres