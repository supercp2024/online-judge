CXX := g++
SRC_DIR := .
MODE ?= debug
BUILD_DIR := build/$(MODE)
BIN_DIR := bin/$(MODE)
TARGET := $(BIN_DIR)/examiner_server.out

SRCS := main.cpp code_file.cpp protocol.cpp task.cpp worker.cpp server.cpp my_utility.cpp log.cpp
OBJS := $(SRCS:%.cpp=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

# 编译选项根据构建类型设置
ifeq ($(MODE),release)
CXXFLAGS := -O3 -DNDEBUG -std=c++20 -I$(SRC_DIR)
else
CXXFLAGS := -g -O0 -std=c++20 -I$(SRC_DIR)
endif

LDFLAGS := -ljsoncpp -pthread ../log_pro_max_plus/libmy_log.a

# 无对应源文件的头文件
HEADER_DEPS := setting.hpp socket.hpp

.PHONY: all release debug clean build

all: debug

release:
	@$(MAKE) build MODE=release

debug:
	@$(MAKE) build MODE=debug

build: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# 通用编译规则
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -MMD -MP -Wall -Wextra -c $< -o $@

-include $(DEPS)

clean:
	rm -rf build bin