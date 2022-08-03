#!/bin/sh

# pull new image
docker pull markbeep/cup:latest

# stop old container
docker stop cup || true
docker rm cup || true

# run new build
docker run -d --env-file .env --restart unless-stopped -v data:/app/data --name cup markbeep/cup

# removes the old unused image
docker image prune -f
