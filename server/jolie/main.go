package main

import (
	"github.com/thethingsnetwork/server-shared"
	"log"
)

var (
	consumer Consumer
	mqtt     PacketHandler
	database PacketHandler
)

func main() {
	log.Printf("Jolie is %s", "ALIVE")

	err := connectConsumer()
	if err != nil {
		log.Fatalf("Failed to connect consumer: %s", err.Error())
	}
	log.Printf("Connected to consumer: ")

	queues, err := consumer.Consume()
	if err != nil {
		log.Fatalf("Failed to consume queues: %s", err.Error())
	}
	log.Printf("Consume queues")

	database, err = connectDatabase(queues)
	if err != nil {
		log.Fatalf("Failed to connect database: %s", err.Error())
	}
	log.Printf("Connected to MongoDB")

	mqtt, err = connectMqtt(queues)
	if err != nil {
		log.Fatalf("Failed to connect MQTT: %s", err.Error())
	}
	log.Print("Connected to Paho")

	go Handle(queues, []PacketHandler{database, mqtt})

	select {}
}

func connectConsumer() error {
	var err error
	consumer, err = ConnectRabbitConsumer()
	if err != nil {
		return err
	}

	err = consumer.Configure()
	if err != nil {
		return err
	}

	return nil
}

func connectDatabase(queues *shared.ConsumerQueues) (PacketHandler, error) {
	var err error
	database, err = ConnectMongoDatabase()
	if err != nil {
		return nil, err
	}

	return database, nil
}

func connectMqtt(queues *shared.ConsumerQueues) (PacketHandler, error) {
	var err error
	mqtt, err = ConnectPaho()
	if err != nil {
		return nil, err
	}

	return mqtt, nil
}

func Handle(queues *shared.ConsumerQueues, handlers []PacketHandler) {
	for {
		select {
		case status := <-queues.GatewayStatuses:
			for _, h := range handlers {
				h.HandleStatus(status)
			}
		case packet := <-queues.RxPackets:
			for _, h := range handlers {
				h.HandlePacket(packet)
			}
		}
	}
}
