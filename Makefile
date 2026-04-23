PI_HOST := mletemple@minitel
PI_DIR := ~

CXX      = arm-linux-gnueabihf-g++
CXXFLAGS = --sysroot=$(SYSROOT) -I$(SYSROOT)/usr/include -std=c++2a -Wall -Wextra -pthread -I include -march=armv6 -mcpu=arm1176jzf-s -mfpu=vfp -mfloat-abi=hard -marm -g
LDFLAGS  = --sysroot=$(SYSROOT) -Wl,--as-needed -Wl,--allow-shlib-undefined -pthread -lavcodec -lavformat -lavutil -lswscale

TARGET  = wm-server
SRCDIR  = src
OBJDIR  = obj

SRCS = $(wildcard $(SRCDIR)/*.cpp)
OBJS = $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(SRCS))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

deploy: all
	scp $(TARGET) $(PI_HOST):$(PI_DIR)/

run: deploy
	ssh $(PI_HOST) "$(PI_DIR)/$(notdir $(TARGET))"

clean:
	rm -rf $(OBJDIR) $(TARGET)

.PHONY: all clean run