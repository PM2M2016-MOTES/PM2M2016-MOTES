#!/bin/sh

echo "*** Starting server ...***"
cd server-devenv;
sudo docker-compose start && cd ../visualization;
sudo npm start;
