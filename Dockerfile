FROM ubuntu:22.04

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

# install kubectl
RUN curl -LO https://dl.k8s.io/release/v1.26.5/bin/linux/amd64/kubectl
RUN curl -LO https://dl.k8s.io/release/v1.26.5/bin/linux/amd64/kubectl.sha256
RUN echo "$(cat kubectl.sha256)  kubectl" | sha256sum --check
RUN install -o root -g root -m 0755 kubectl /usr/local/bin/kubectl

COPY entrypoint.sh /home/postgres/entrypoint.sh

ENTRYPOINT [ "/home/postgres/entrypoint.sh" ]