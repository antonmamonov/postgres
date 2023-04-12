#!/bin/bash

IMAGENAME=antonm/postgres:v0.0.1
CONTAINERNAME=pg

docker rm -f $CONTAINERNAME

docker run -t -d \
    --name $CONTAINERNAME \
    --network=host \
    --entrypoint=/bin/bash \
    -v $PWD:/home/postgres/workdir \
    -e POSTGRES_HOST=$POSTGRES_HOST \
    -e PGPASSWORD=$PGPASSWORD \
    $IMAGENAME

docker exec -it $CONTAINERNAME /bin/bash