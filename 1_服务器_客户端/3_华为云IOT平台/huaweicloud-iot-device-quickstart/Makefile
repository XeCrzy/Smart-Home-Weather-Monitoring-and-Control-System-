CC = gcc
CFLAGS = -g -w -lrt -m64 -Wl,-z,relro,-z,now,-z,noexecstack -fno-strict-aliasing -fno-omit-frame-pointer -pipe -Wall -fPIC -MD -MP -fno-common -freg-struct-return  -fno-inline -fno-exceptions -Wfloat-equal -Wshadow -Wformat=2 -Wextra -rdynamic -Wl,-z,relro,-z,noexecstack -fstrength-reduce -fno-builtin -fsigned-char -ffunction-sections -fdata-sections -Wpointer-arith -Wcast-qual -Waggregate-return -Winline -Wunreachable-code -Wcast-align -Wundef -Wredundant-decls  -Wstrict-prototypes -Wmissing-prototypes -Wnested-externs -pthread
# -D _SYS_LOG=1 -shared -fPIC
#-D Linux=1
CXXFLAGS = -O2 -g -Wall -fmessage-length=0 -lrt -m64 -Wl,-z,relro,-z,now,-z,noexecstack -fno-strict-aliasing -fno-omit-frame-pointer -pipe -Wall -fPIC -MD -MP -fno-common -freg-struct-return  -fno-inline -fno-exceptions -Wfloat-equal -Wshadow -Wformat=2 -Wextra -rdynamic -Wl,-z,relro,-z,noexecstack -fstack-protector-strong -fstrength-reduce -fno-builtin -fsigned-char -ffunction-sections -fdata-sections -Wpointer-arith -Wcast-qual -Waggregate-return -Winline -Wunreachable-code -Wcast-align -Wundef -Wredundant-decls  -Wstrict-prototypes -Wmissing-prototypes -Wnested-externs

OBJS = AgentLiteDemo.o client_c.o

#$(warning "OS $(OS)")
#$(warning "OSTYPE $(OSTYPE)")

HEADER_PATH = -I./include
LIB_PATH = -L./lib
SRC_PATH = ./src

LIBS = $(LIB_PATH) -lpaho-mqtt3as $(LIB_PATH) -lHWMQTT -lpthread
#$(LIB_PATH) -lpaho-mqtt3cs $(LIB_PATH)

ifeq ($(OS), Windows_NT)
	TARGET = MQTT_Demo.exe
else 
#	TARGET = libHWMQTT.so
	TARGET = MQTT_Demo.o
endif

$(TARGET):	$(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

AgentLiteDemo.o: AgentLiteDemo.c
	$(CC) $(CFLAGS) -c AgentLiteDemo.c -o AgentLiteDemo.o $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/cjson/
client_c.o: client_c.c client_c.h
	$(CC) $(CFLAGS) -c client_c.c -o client_c.o $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/cjson/
all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET) *.d
