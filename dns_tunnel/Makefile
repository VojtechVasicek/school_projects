sender: ./sender/dns_sender.c
	gcc ./sender/dns_sender.c ./shared/packet.c  ./shared/base32.c ./sender/dns_sender_events.c -o dns_sender

receiver: ./receiver/dns_receiver.c
	gcc ./receiver/dns_receiver.c ./shared/base32.c ./shared/packet.c ./receiver/dns_receiver_events.c -o dns_receiver

all: ./sender/dns_sender.c ./receiver/dns_receiver.c
	gcc ./sender/dns_sender.c ./shared/base32.c ./shared/packet.c ./sender/dns_sender_events.c -o dns_sender
	gcc ./receiver/dns_receiver.c ./shared/base32.c ./shared/packet.c ./receiver/dns_receiver_events.c -o dns_receiver
