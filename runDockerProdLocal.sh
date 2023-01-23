#!/bin/bash

IMAGENAME=antonm/postgres:v0.0.1
CONTAINERNAME=pg

docker rm -f $CONTAINERNAME

docker run -t -d \
    --name $CONTAINERNAME \
    -p 5432:5432 \
    -e PGPASSWORD=$PGPASSWORD \
    $IMAGENAME

docker logs -f $CONTAINERNAME