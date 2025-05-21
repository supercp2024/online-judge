CXX := g++
SRC_DIR := .
MODE ?= debug
BUILD_DIR := build/$(MODE)
BIN_DIR := bin/$(MODE)
TARGET := $(BIN_DIR)/filler.out

SRCS := main.cpp database.cpp
OBJS := $(SRCS:%.cpp=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

# 编译选项根据构建类型设置
ifeq ($(MODE),release)
CXXFLAGS := -O3 -DNDEBUG -std=c++20 -I$(SRC_DIR)
else
CXXFLAGS := -g -O0 -std=c++20 -I$(SRC_DIR)
endif

LDFLAGS := -ljsoncpp -I/usr/local/mysqlcppconn/include -L/usr/local/mysqlcppconn/lib -lmysqlcppconn

# 无对应源文件的头文件
HEADER_DEPS := setting.hpp socket.hpp

.PHONY: all release debug clean

all: debug

# 直接构建模式目标（关键修正：移除递归调用）
release: export MODE = release
release: $(TARGET)

debug: export MODE = debug
debug: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# 通用编译规则
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

# 显式声明关键头文件依赖
$(BUILD_DIR)/main.o: database.hpp
$(BUILD_DIR)/database.o: database.hpp

-include $(DEPS)

clean:
	rm -rf build bin