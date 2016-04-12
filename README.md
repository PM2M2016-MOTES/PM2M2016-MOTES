# PM2M2016-MOTES

This repo has two parts : A `client` part and a `server` part.

The `client` part contains two softwares whiches allow the user to process data from its own computer and send them on a LoRaWAN. This data are send through a Motes Wyres.

The data are then send to Amazon Server using UDP packets.

These packets are recieved by the local version of `croft` and are send to `jolie` through RabbitMQ.

The data are then stored into `MongoDB` and can be visualized using the `visualization` tool of the repo.

