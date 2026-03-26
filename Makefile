CC_WIN = x86_64-w64-mingw32-gcc

CFLAGS_WIN = -Os -nostdlib -s -Iinclude -Wl,-e,WinMainCRTStartup,--gc-sections
LDFLAGS_WIN = -lkernel32

OBJ_DIR := build_temp
SRC_DIR := src

OUT_NAME := qmcg.exe

SRCS := $(wildcard src/*.c) \
		#$(wildcard src/windows/*.c)

OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRCS))

.PHONY: all clean

all: compile

compile: $(OBJS)
	$(CC_WIN) $(CFLAGS_WIN) -o $(OUT_NAME) $^ $(LDFLAGS_WIN)

$(OBJ_DIR)/%.o: %.c
	@if not exist "$(dir $@)" mkdir "$(dir $@)"
	$(CC_WIN) $(CFLAGS_WIN) -c $< -o $@

clean:
	-del /Q $(OBJ_DIR)\*.o $(OUT_NAME) $(UPX_NAME)
	-rmdir /Q /S $(OBJ_DIR)