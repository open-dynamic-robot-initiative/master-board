
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

%.o: tests/%.cpp
	mkdir -p build
	pkg-config --atleast-version=3.1 catch2
	$(CC) $(CFLAGS) $(INCLUDES) $(shell pkg-config --cflags catch2) -std=c++17 -o build/$@ -c $^

app: example.o ESPNOW_manager.o ESPNOW_types.o Link_manager.o ETHERNET_types.o master_board_interface.o motor.o motor_driver.o
	mkdir -p bin
	$(CC) $(CFLAGS) -o bin/example $(addprefix build/,$^) -pthread

app_pd: example_pd.o ESPNOW_manager.o ESPNOW_types.o Link_manager.o ETHERNET_types.o master_board_interface.o motor.o motor_driver.o
	mkdir -p bin
	$(CC) $(CFLAGS) -o bin/example_pd $(addprefix build/,$^) -pthread

sdk_tests: test_protocol.o
	mkdir -p bin
	$(CC) $(CFLAGS) -o bin/sdk_tests $(addprefix build/,$^) $(shell pkg-config --libs catch2) -lCatch2Main -pthread

test : sdk_tests
	bin/sdk_tests

all: clear clean app app_pd

clean: 
	#$(RM) build/*
	$(RM) bin/*

clear:
	clear
