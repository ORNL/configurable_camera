CC=gcc
CPPFLAGS := -Iinclude

PKG=`pkg-config --cflags --libs gstreamer-rtsp-server-1.0 gstreamer-rtsp-1.0 gstreamer-video-1.0 gstreamer-1.0 glib-2.0`
CFLAGS=-g -O2 -Wno-deprecated-declarations $(PKG)
DEV_CFLAGS=-g -Wall -Wno-deprecated-declarations $(PKG)

SRC_DIR := src
SRC := $(wildcard $(SRC_DIR)/*.c)
SENSOR_SETUP_SRC := src/sensor.c src/cJSON.c src/log.c other/sensor_setup.c
BUILD := build

# Define TEST_REQS in each test, add test recipe to test_all
TEST = $(CC) $(CPPFLAGS) $(TEST_REQS) tests/$@.c -o $(BUILD)/$@ $(DEV_CFLAGS)

build:
	mkdir -p build


main:
	$(CC) $(CPPFLAGS) $(SRC) -o main $(CFLAGS)
sensor_setup:
	$(CC) $(CPPFLAGS) $(SENSOR_SETUP_SRC) -o sensor_setup $(CFLAGS)


all: build main sensor_setup

dev_main: build
	$(CC) $(CPPFLAGS) $(SRC) -o main $(DEV_CFLAGS)

dev_sensor_setup: build
	$(CC) $(CPPFLAGS) $(SENSOR_SETUP_SRC) -o sensor_setup $(DEV_CFLAGS)

dev_all: build dev_main dev_sensor_setup

# TODO: clean up leftover recipes

mvp: build
	$(CC) $(CPPFLAGS) src/mvp.c src/config.c src/cJSON.c src/statusbroadcaster.c src/pipeline.c src/sensor.c -o $(BUILD)/mvp $(DEV_CFLAGS)

test_config: build
	$(CC) $(CPPFLAGS) src/config.c src/cJSON.c src/addresschecker.c tests/test_config.c -o $(BUILD)/test_config -g

test_statusbroadcaster: build
	$(CC) $(CPPFLAGS) src/statusbroadcaster.c src/config.c src/cJSON.c tests/test_statusbroadcaster.c -o $(BUILD)/test_statusbroadcaster $(DEV_CFLAGS)

test_pipeline_creation: build
	$(CC) $(CPPFLAGS) src/sensor.c src/pipeline.c tests/$@.c -o $(BUILD)/$@ $(DEV_CFLAGS)

test_sensor: build
	$(CC) $(CPPFLAGS) src/sensor.c src/log.c src/cJSON.c tests/$@.c -o $(BUILD)/$@ $(DEV_CFLAGS)

test_sensor_setup: build
	$(CC) $(CPPFLAGS) src/sensor.c src/cJSON.c tests/$@.c -o $(BUILD)/$@ $(DEV_CFLAGS)

test_csv: build 
	$(CC) $(CPPFLAGS) src/pipeline.c src/sensor.c src/config.c src/cJSON.c tests/$@.c -o $(BUILD)/$@ $(DEV_CFLAGS)

test_all: test_csv, test_sensor, test_config


clean: build
	rm -rf $(BUILD)
	rm ./main
	rm ./sensor_setup
