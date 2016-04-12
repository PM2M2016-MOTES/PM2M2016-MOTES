package main

import (
	"bytes"
	"encoding/base64"
	"encoding/binary"
	"errors"
	"fmt"
	"log"
	"net"
	"time"

	"github.com/thethingsnetwork/croft/lora"
	"github.com/thethingsnetwork/server-shared"
)

func readUDPMessages(port int, messages chan interface{}) {
	addr, err := net.ResolveUDPAddr("udp", fmt.Sprintf("0.0.0.0:%d", port))
	if err != nil {
		log.Fatalf("Failed to resolve address: %s", err.Error())
	}

	listener, err := net.ListenUDP("udp", addr)
	if err != nil {
		log.Fatalf("Failed to start UDP server: %s", err.Error())
	}
	defer listener.Close()

	conn := lora.NewConn(listener)
	for {
		msg, err := conn.ReadMessage()
		if err != nil {
			log.Printf("Failed to read message: %s", err.Error())
			continue
		}
		go handleMessage(msg, messages)
	}
}

func handleMessage(msg *lora.Message, messages chan interface{}) {
	switch msg.Header.Identifier {
	case lora.PUSH_DATA:
		publishPushMessagePayloads(msg.GatewayEui, msg.Payload.(lora.PushMessagePayload), messages)
	}

	err := msg.Ack()
	if err != nil {
		log.Printf("Failed to send ACK: %s", err.Error())
	}
}

func publishPushMessagePayloads(gatewayEui []byte, payload lora.PushMessagePayload, messages chan interface{}) {
	if payload.Stat != nil {
		stat, err := convertStat(gatewayEui, payload.Stat)
		if err != nil {
			log.Printf("Failed to convert Stat: %s", err.Error())
		} else {
			messages <- stat
		}
	}

	if payload.RXPK != nil {
		for _, rxpk := range payload.RXPK {
			packet, err := convertRXPK(gatewayEui, rxpk)
			if err != nil {
				log.Printf("Failed to convert RXPK: %s", err.Error())
				continue
			}
			messages <- packet
		}
	}
}

func convertStat(gatewayEui []byte, stat *lora.Stat) (*shared.GatewayStatus, error) {
	t, err := time.Parse(time.RFC822, stat.Time)
	if err != nil {
		log.Printf("Failed to parse time %s: %s", stat.Time, err.Error())
		t = time.Now()
	}

	return &shared.GatewayStatus{
		Eui:               fmt.Sprintf("%X", gatewayEui),
		Time:              t,
		Latitude:          &stat.Lati,
		Longitude:         &stat.Long,
		Altitude:          &stat.Alti,
		RxCount:           &stat.Rxnb,
		RxOk:              &stat.Rxok,
		RxForwarded:       &stat.Rxfw,
		AckRatio:          &stat.Ackr,
		DatagramsReceived: &stat.Dwnb,
		DatagramsSent:     &stat.Txnb,
	}, nil
}

func convertRXPK(gatewayEui []byte, rxpk *lora.RXPK) (*shared.RxPacket, error) {
	data, err := rxpk.ParseData()
	if err != nil {
		log.Printf("Failed to parse RXPK: %s", err.Error())
		return nil, err
	}

	networkKey, err := getNetworkKey(gatewayEui, data.DevAddr)
	if err != nil {
		log.Printf("Failed to get key (gateway %X, node %X): %s", gatewayEui, data.DevAddr, err.Error())
		return nil, err
	}

	var key []byte
	if data.FPort == 0 {
		key = networkKey
	} else {
		key, err = getAppKey(gatewayEui, data.DevAddr)
		if err != nil {
			log.Printf("Failed to get key (gateway %X, node %X): %s", gatewayEui, data.DevAddr, err.Error())
			return nil, err
		}
	}
<<<<<<< HEAD
	log.Printf("Key = %X", key)
=======
	log.Printf("Key = %s", key)
>>>>>>> 4d9689344ad025c693150e920e11580f50b5d353

	ok, err := data.TestIntegrity(networkKey)
	if err != nil {
		log.Printf("Failed to test integrity: %s", err.Error())
		return nil, err
	}
	if !ok {
		return nil, errors.New("Integrity test failed")
	}

<<<<<<< HEAD
	log.Printf("Je vais dechiffrer")
=======
>>>>>>> 4d9689344ad025c693150e920e11580f50b5d353
	payload, err := data.DecryptPayload(key)
	if err != nil {
		log.Printf("Error decrypting data: %s", err.Error())
		return nil, err
	}
<<<<<<< HEAD
	log.Printf("Si j ai fait mon boulot les donnees sont visible")
	log.Printf("%X",payload)
=======
>>>>>>> 4d9689344ad025c693150e920e11580f50b5d353

	return &shared.RxPacket{
		GatewayEui: fmt.Sprintf("%X", gatewayEui),
		NodeEui:    fmtDevAddr(data.DevAddr),
		Time:       time.Now(), // rxpk.Time,
		Frequency:  &rxpk.Freq,
		DataRate:   rxpk.Datr,
		Rssi:       &rxpk.Rssi,
		Snr:        &rxpk.Lsnr,
		RawData:    rxpk.Data,
		Data:       base64.StdEncoding.EncodeToString(payload),
	}, nil
}

func fmtDevAddr(devAddr uint32) string {
	buf := new(bytes.Buffer)
	binary.Write(buf, binary.BigEndian, devAddr)
	return fmt.Sprintf("%X", buf.Bytes())
}

func getNetworkKey(gatewayEui []byte, devAddr uint32) ([]byte, error) {
	// TODO: Implement fetching the right network key. Now returning Semtech's default key
	key := []byte{0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C}
	return key, nil
}

func getAppKey(gatewayEui []byte, devAddr uint32) ([]byte, error) {
	// TODO: Implement fetching the right application key
	//return getNetworkKey(gatewayEui, devAddr)
<<<<<<< HEAD
	key := []byte{0x10, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01}
=======
	key := []byte{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
>>>>>>> 4d9689344ad025c693150e920e11580f50b5d353
	return key, nil
}
