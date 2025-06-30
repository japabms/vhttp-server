CC = clang
CFLAGS_DEBUG = -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wdouble-promotion -Wnull-dereference -fno-common -ggdb -O0
CFLAGS_RELEASE = -Wall -Wextra -Wpedantic -Wshadow -Wconversion -O2 -march=native
LIBS = -lpthread
APP_NAME = main

OBJS = $(patsubst %.c,$(BUILD_DIR)%.o,$(wildcard *.c))

BUILD_DIR = ./build/

$(shell mkdir -p $(BUILD_DIR))

all: debug

release: CFLAGS = $(CFLAGS_RELEASE)
release: $(OBJS)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)$(APP_NAME) $(OBJS) $(LIBS)

debug: CFLAGS = $(CFLAGS_DEBUG)
debug: $(OBJS)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)$(APP_NAME) $(OBJS) $(LIBS)

$(BUILD_DIR)%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(BUILD_DIR)*.o $(BUILD_DIR)$(APP_NAME)

vstring_test: vstring.o vutil.h
	$(CC) $(CFLAGS) ./thinkering/vstring_test.c -o vstring_test $(BUILD_DIR)vfile.o $(BUILD_DIR)vstring.o $(LIBS) -ggdb

recv_test: vtcp_socket.o vutil.h
	$(CC) $(CFLAGS) ./thinkering/recv_test.c -o recv_test $(BUILD_DIR)vtcp_socket.o $(LIBS) -ggdb
