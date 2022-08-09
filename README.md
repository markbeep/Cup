1. Clone repo with submodules: `git clone --recurse-submodules git@github.com:markbeep/Cup.git`
2. Build manually (optional): `docker build -t markbeep/cup .`
3. Create volume: `docker volume create data`
3. Run container: `docker run -d --env-file .env --restart unless-stopped -v data:/app/data --name cup markbeep/cup`
3. Run bash in container: `docker run -it --env-file .env --entrypoint /bin/sh -v data:/app/data --name cup markbeep/cup`

.env file contains `DISCORD_TOKEN=...` with the Discord bot token.

Webhook loop:
1. `sudo -E webhook -hooks autodeploy/hooks.json -verbose --port 80`
