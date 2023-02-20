FROM ubuntu:22.04

RUN apt update &&               \
    apt install -y wrk &&       \
    rm -rf /var/lib/apt/lists/*

ENTRYPOINT [ "wrk" ]
