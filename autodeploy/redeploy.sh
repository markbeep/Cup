#!/bin/sh

# first pull new changes
git pull
git submodule update --recursive --remote

# build new image
sudo docker build -t cup .

# stop old container
sudo docker stop cup || true
sudo docker rm cup || true

# run new build
sudo docker run -d --restart unless-stopped --name cup cup

# removes the old image
sudo docker image prune -f
