# ============================================================================
#  SAILER ray tracer (2026 JSON edition)
#
#  Builds a command-line ray tracer that reads a JSON scene description and
#  writes a PNG / JPEG / BMP / TGA / PPM image.
#
#  Targets:
#    make            build ./ray
#    make run        build, then render scenes/cone.json -> cone.png
#    make clean      remove build artifacts
# ============================================================================

CC      := cc
CSTD    :=
# Permissive flags: this is 1990s K&R-era C compiled by a modern clang.
WARN    := -Wno-implicit-function-declaration -Wno-implicit-int \
           -Wno-deprecated-non-prototype -Wno-int-conversion \
           -Wno-deprecated-declarations -Wno-pointer-sign \
           -Wno-return-type -Wno-parentheses
CFLAGS  := -O2 -g $(CSTD) -Isrc -Ithird_party $(WARN)
LDFLAGS := -lm

BUILD   := build
BIN     := ray

# Engine + new front-end sources (lex.c / parser.c / writeppm.c are gone).
SRCS := \
    src/csg.c src/bound.c src/tri.c src/board.c src/box.c src/conic.c \
    src/sphere.c src/mathfun.c src/raymain.c src/view.c src/init.c \
    src/symtab.c src/attrib.c src/check.c src/range.c src/paramap.c \
    src/mandel.c src/bitmap.c src/global.c src/support.c src/translat.c \
    src/parse_tr.c src/sail.c src/image.c src/jsonscene.c src/main.c \
    third_party/cJSON.c

OBJS := $(patsubst %.c,$(BUILD)/%.o,$(SRCS))

.PHONY: all run clean

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $(BIN)
	@echo "Built ./$(BIN)"

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(BIN)
	./$(BIN) scenes/cone.json -o cone.png

clean:
	rm -rf $(BUILD) $(BIN)
