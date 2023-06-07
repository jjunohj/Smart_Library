
VPATH = _build
objects = rfid.o RC522.o
.PHONY : clear run

top:
	make clear
	make compile

compile: $(objects)
	gcc $(objects) -o rfid -lwiringPi

clear: 
	rm -f rfid
	rm -f $(objects)

run: clear compile
	./rfid

./$(VPATH)/rfid.o:RC522.o
	gcc -c rfid.c

./$(VPATH)/RCC522.o:
	gcc -c RC522.c RC522.h
