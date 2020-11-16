
CC = g++
CFLAGS  = -g -Wall -std=c++11
INCLUDES = -I include/

default: all

%.o: example/%.cpp
	mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDES) -o build/$@ -c $^

%.o: src/%.cpp
	mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDES) -o build/$@ -c $^

app: example.o ESPNOW_manager.o ESPNOW_types.o Link_manager.o ETHERNET_types.o master_board_interface.o motor.o motor_driver.o
	mkdir -p bin
	$(CC) $(CFLAGS) -o bin/example $(addprefix build/,$^) -pthread

app_pd: example_pd.o ESPNOW_manager.o ESPNOW_types.o Link_manager.o ETHERNET_types.o master_board_interface.o motor.o motor_driver.o
	mkdir -p bin
	$(CC) $(CFLAGS) -o bin/example_pd $(addprefix build/,$^) -pthread

all: clear clean app app_pd

clean: 
	#$(RM) build/*
	$(RM) bin/*

clear:
	clear
