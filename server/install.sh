#!/bin/sh

echo "*** Installing server, this may take some time ***"
echo "*** cd server-devenv ***"
cd server-devenv;
echo "*** sudo docker-compose pull ***"
sudo docker-compose pull;
echo "*** sudo docker-compose build ***"
sudo docker-compose build;
sudo docker-compose create;
echo "*** Done. ***"
