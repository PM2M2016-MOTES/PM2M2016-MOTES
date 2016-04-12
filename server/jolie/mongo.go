package main

import (
	"fmt"
	"log"
	"os"
	"time"

	"github.com/thethingsnetwork/server-shared"
	"gopkg.in/mgo.v2"
)

const (
	MONGODB_ATTEMPTS = 20
)

type MongoDatabase struct {
	session *mgo.Session
}

type PersonTest struct {
	Name string
	Phone string
}

func ConnectMongoDatabase() (PacketHandler, error) {
	var err error
	uri := os.Getenv("MONGODB_URI")
	for i := 0; i < MONGODB_ATTEMPTS; i++ {
		var session *mgo.Session
		session, err = mgo.Dial(fmt.Sprintf("%s:27017", uri))
		if err != nil {
			log.Printf("Failed to connect to %s: %s", uri, err.Error())
			time.Sleep(time.Duration(2) * time.Second)
		} else {
			log.Printf("Connected to %s", uri)
			session.SetMode(mgo.Monotonic, true)
			session.SetSocketTimeout(time.Duration(6) * time.Second)
			session.SetSyncTimeout(time.Duration(6) * time.Second)

		//Tests in progress
		test_insert := session.DB("jolie").C("test_insert")
		err = test_insert.Insert(&PersonTest{"Alice", "01 47 20 00 01"},
				&PersonTest{"Bob", "02 47 20 00 02"})
		if err != nil {
			log.Printf("Il y a eu une erreur !!! : %s", err.Error())
		}
		log.Printf("Insertion done")

			return &MongoDatabase{session}, nil
		}
	}

	return nil, err
}

func (db *MongoDatabase) Configure() error {
	return nil
}

func (db *MongoDatabase) HandleStatus(status *shared.GatewayStatus) {
	err := db.session.DB("jolie").C("gateway_statuses").Insert(status)
	if err != nil {
		log.Printf("Failed to save status: %s", err.Error())
	} else {
		log.Printf("Insertion into \"gateway_statuses\"")
	}
}

func (db *MongoDatabase) HandlePacket(packet *shared.RxPacket) {
	err := db.session.DB("jolie").C("rx_packets").Insert(packet)
	if err != nil {
		log.Printf("Failed to save packet: %s", err.Error())
	} else {
		log.Printf("Insertion into \"rx_packets\"")
	}
}

func (db *MongoDatabase) RecordGatewayStatus(status *shared.GatewayStatus) error {
	return nil
}

func (db *MongoDatabase) RecordRxPacket(packet *shared.RxPacket) error {
	return nil
}
