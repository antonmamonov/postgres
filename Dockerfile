FROM ubuntu:20.04

RUN apt-get update --fix-missing -y
# needed for development
RUN apt-get install vim wget curl gnupg -y

# need for building postgres
RUN apt-get install build-essential gcc -y

# additional packages
RUN apt-get install libreadline-dev zlib1g-dev flex bison -y

WORKDIR /postgresworkdir
COPY postgres /postgresworkdir/postgres

# build postgres
RUN cd postgres && ./configure && make && make install

# clean up source files
RUN rm -rf postgres

RUN useradd -m postgres
WORKDIR /home/postgres
USER postgres
RUN mkdir /home/postgres/data
ENV PATH="${PATH}:/usr/local/pgsql/bin"

# postgres related environment variables
ENV PGDATA="/home/postgres/data"

COPY entrypoint.sh /home/postgres/entrypoint.sh

ENTRYPOINT [ "/home/postgres/entrypoint.sh" ]