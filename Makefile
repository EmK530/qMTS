CC_WIN = x86_64-w64-mingw32-gcc

VERSION = "1.0.0"

CFLAGS_WIN = -O2 -nostdlib -s -Iinclude -Wl,-e,__main,--gc-sections -fno-asynchronous-unwind-tables -DVERSION=\"$(VERSION)\"
CFLAGS_WIN_TINY = -Wl,--gc-sections,--file-alignment=0x1,--section-alignment=0x1 -ffunction-sections -fdata-sections
LDFLAGS_WIN = -lkernel32 -lshell32 -lcomdlg32

OBJ_DIR := build_temp
SRC_DIR := src

OUT_NAME := qmts.exe

SRCS := $(wildcard src/*.c)

OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRCS))

.PHONY: all clean

all: compile

compile: $(OBJS)
	$(CC_WIN) $(CFLAGS_WIN) -o $(OUT_NAME) $^ $(LDFLAGS_WIN)

tiny: CFLAGS_WIN += $(CFLAGS_WIN_TINY)
tiny: clean compile

$(OBJ_DIR)/%.o: %.c
	@mkdir -p "$(dir $@)"
	$(CC_WIN) $(CFLAGS_WIN) -c $< -o $@

clean:
	-rm -f $(OUT_NAME)
	-rm -rf $(OBJ_DIR)