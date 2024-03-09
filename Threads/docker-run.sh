#/bin/bash

PWD=$(pwd)

echo "Creating a container and attaching it to current shell"
docker run -it --rm -v "$PWD":/home/os-fall-2023 os-fall-2023:latest