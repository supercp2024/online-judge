CXX := g++
SRC_DIR := .
MODE ?= release
BUILD_DIR = build/$(MODE)
BIN_DIR = bin/$(MODE)
TARGET = $(BIN_DIR)/log.out
LIB_STATIC = $(BIN_DIR)/liblog.a
LIB_SHARED = $(BIN_DIR)/liblog.so

SRCS := logger.cpp format.cpp main.cpp
SRCS_LIB := logger.cpp format.cpp
OBJS_SHARED = $(SRCS_LIB:%.cpp=$(BUILD_DIR)/shared/%.o)
OBJS_STATIC = $(SRCS_LIB:%.cpp=$(BUILD_DIR)/%.o)
OBJS = $(SRCS:%.cpp=$(BUILD_DIR)/%.o)
DEPS = $(OBJS:.o=.d) $(OBJS_SHARED:.o=.d)

# 编译选项根据构建类型设置
ifeq ($(MODE),release)
CXXFLAGS = -O3 -DNDEBUG -std=c++20 -I$(SRC_DIR)
else
CXXFLAGS = -g -O0 -std=c++20 -Wall -I$(SRC_DIR)
endif

# 无对应源文件的头文件
HEADER_DEPS := setting.hpp compatible.hpp message.hpp output.hpp level.hpp static shared

.PHONY: all release debug clean build

all: debug

release:
	@$(MAKE) build MODE=release

debug:
	@$(MAKE) build MODE=debug

build: $(TARGET)

static: $(LIB_STATIC)

shared: $(LIB_SHARED)

$(TARGET): $(OBJS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -Wall -Wextra $^ -o $@

$(LIB_STATIC): $(OBJS_STATIC)
	@mkdir -p $(@D)
	ar rcs $@ $^

$(LIB_SHARED): $(OBJS_SHARED)
	@mkdir -p $(@D)
	$(CXX) -shared $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -MMD -MP -Wall -Wextra -c $< -o $@

$(BUILD_DIR)/shared/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -fPIC -MMD -MP -Wall -Wextra -c $< -o $@

-include $(DEPS)

clean:
	rm -rf build bin