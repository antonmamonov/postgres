#!/bin/bash

echo "Starting Anton PostgreSQL"
date

# check if the data directory has postgresql.conf
if [ ! -f $PGDATA/postgresql.conf ]; then
    echo "Data directory is empty"
    echo "Initializing database"
    initdb   
fi

postgres