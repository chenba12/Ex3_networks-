.PHONY: clean all
all:  sender receiver
sender: Sender.c
	gcc -g Sender.c -o sender
receiver: Receiver.c
	gcc -g Receiver.c -o receiver
clean:
	rm -f sender receiver