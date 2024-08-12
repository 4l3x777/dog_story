docker build -t game_server . &&
docker run -it --rm -p 80:8080 game_server
