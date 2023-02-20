#!/bin/bash

set -eou pipefail

DOCKER_NETWORK_NAME="benchmark-net"
SERVER_CONTAINER_NAME="v8-example-server"

function ensure_docker_images() {
    docker image build -t v8-example ./
    docker image build -t wrk ./wrk
}

function start_benchmark() {
    docker network create "$DOCKER_NETWORK_NAME" || true
    docker container run -d --name "$SERVER_CONTAINER_NAME" --network "$DOCKER_NETWORK_NAME" v8-example
    docker container run --network "$DOCKER_NETWORK_NAME" wrk -t4 -c100 -d30s "http://$SERVER_CONTAINER_NAME:8080/hello-world.js"
}

function cleanup() {
    echo "Cleaning up. Please be patient..."
    docker container stop "$SERVER_CONTAINER_NAME" >/dev/null 2>/dev/null
    docker container rm "$SERVER_CONTAINER_NAME" >/dev/null 2>/dev/null
    docker network rm "$DOCKER_NETWORK_NAME" >/dev/null 2>/dev/null
}

ensure_docker_images
start_benchmark
cleanup
