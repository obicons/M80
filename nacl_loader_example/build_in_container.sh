#!/bin/bash

set -eou pipefail

IMAGE_NAME=nacl-loader-build-image

if [[ "$(docker image ls "$IMAGE_NAME" | wc -l)" -eq 1 ]]; then
    docker image build -t "$IMAGE_NAME" ./
fi

docker container run --user $UID:"$(id -g)" -v "$(pwd)":/src/ --workdir /src/ $IMAGE_NAME  make
