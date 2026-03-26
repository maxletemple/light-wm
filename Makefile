PI_HOST := minitel
PI_DIR := ~

CXX      = armv6-unknown-linux-gnueabihf-g++
CXXFLAGS = -std=c++20 -Wall -Wextra -pthread -I include -march=armv6 -mcpu=arm1176jzf-s -mfpu=vfp -mfloat-abi=hard  -marm -g
LDFLAGS  = -pthread

TARGET  = wm-server
SRCDIR  = src
OBJDIR  = obj

SRCS = $(wildcard $(SRCDIR)/*.cpp)
OBJS = $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(SRCS))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^

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