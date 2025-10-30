# 统一的Makefile，适用于POSIX和Windows平台

# 编译器设置
CC = g++
CFLAGS = -Wall -Wextra -O2

# 根据系统环境选择源文件和目标文件
# 在Windows上，使用windows特定的源文件和.exe扩展名
# 在其他系统上，使用POSIX源文件

# 检测操作系统
ifneq (,$(findstring Windows,$(OS)))  # Windows环境
    TARGET = ai-api.exe
    SRC = ai-api-c_windows.cpp
    CFLAGS += -DWIN32
    LDFLAGS = -lws2_32 -lssl -lcrypto
    # Windows平台的rm命令
    RM = del /q
else  # POSIX环境(Linux/MacOS等)
    TARGET = ai-api
    SRC = ai-api-c_posix.cpp
    LDFLAGS = -lssl -lcrypto
    # POSIX平台的rm命令
    RM = rm -f
endif

# 构建目标
all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

# 清理目标
clean:
	$(RM) $(TARGET)

.PHONY: all clean