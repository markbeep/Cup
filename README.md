1. `git clone --recurse-submodules git@github.com:markbeep/Cup.git`
2. `docker build --no-cache -t cup .`
3. `docker run -d --restart unless-stopped cup`
