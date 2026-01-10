CXX := g++
CXXFLAGS := -std=c++20 -Wall -fPIC
PROJECT := tabfix_client
SRC_DIR := src
BIN_DIR := bin

SOURCES := $(SRC_DIR)/main.cpp

GIO_CFLAGS := $(shell pkg-config --cflags gio-2.0)
GIO_LIBS := $(shell pkg-config --libs gio-2.0)
QT6_CFLAGS := $(shell pkg-config --cflags Qt6Widgets Qt6DBus)
QT6_LIBS := $(shell pkg-config --libs Qt6Widgets Qt6DBus)
MOC := /usr/lib/qt6/moc

CXXFLAGS += $(GIO_CFLAGS) $(QT6_CFLAGS) -I$(BIN_DIR)
LDFLAGS := $(GIO_LIBS) $(QT6_LIBS)

TARGET := $(BIN_DIR)/$(PROJECT)
MOC_FILE := $(BIN_DIR)/moc_main.cpp

all: $(TARGET)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(MOC_FILE): $(SOURCES) | $(BIN_DIR)
	$(MOC) $(QT6_CFLAGS) $(SOURCES) -o $@

$(TARGET): $(SOURCES) $(MOC_FILE) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(SOURCES) $(LDFLAGS) -o $@

clean:
	rm -rf $(BIN_DIR)

.PHONY: all clean
