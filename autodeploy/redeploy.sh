#!/bin/sh

# first pull new changes
git pull
git submodule update --recursive --remote

# build new image
docker build -t cup .

# stop old container
docker stop cup || true
docker rm cup || true

# run new build
docker run -d --restart unless-stopped --name cup cup

# removes the old image
docker image prune -f
