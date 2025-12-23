# 服务器和客户端A的可执行文件
SERVER_EXE = server
CLIENT_A_EXE = client_A

# 源文件
SERVER_SRC = 2_tcp_server_多线程并发.c
CLIENT_A_SRC = client_A.c forecast.c

# 库目录
CJSON_DIR = cJSON
NETWRAP_DIR = netwrap

# 编译选项
CC = gcc
CFLAGS = -Wall -g -I$(CJSON_DIR) -I$(NETWRAP_DIR)
LDFLAGS = -pthread
CLIENT_A_LIBS = -L$(CJSON_DIR) -lcjson -L$(NETWRAP_DIR) -lvnet -pthread

# 默认目标
all: $(SERVER_EXE) $(CLIENT_A_EXE)

# 编译服务器
$(SERVER_EXE): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# 编译客户端A，并设置rpath，使得程序运行时可以在当前目录的netwrap子目录中找到libvnet.so
$(CLIENT_A_EXE): $(CLIENT_A_SRC) $(CJSON_DIR)/libcjson.a $(NETWRAP_DIR)/libvnet.so
	$(CC) $(CFLAGS) -o $@ $(filter %.c, $^) $(CLIENT_A_LIBS) -Wl,-rpath='$$ORIGIN/netwrap'

# 构建cJSON库
$(CJSON_DIR)/libcjson.a:
	$(MAKE) -C $(CJSON_DIR)

# 构建netwrap库
$(NETWRAP_DIR)/libvnet.so:
	$(MAKE) -C $(NETWRAP_DIR)

# 安装库
install:
	$(MAKE) -C $(CJSON_DIR)
	$(MAKE) -C $(NETWRAP_DIR)

# 清理
clean:
	rm -f $(SERVER_EXE) $(CLIENT_A_EXE)

distclean: clean
	$(MAKE) -C $(CJSON_DIR) clean
	$(MAKE) -C $(NETWRAP_DIR) clean

.PHONY: all install clean distclean