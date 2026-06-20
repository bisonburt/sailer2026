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
LDFLAGS := -lm -framework Metal -framework Foundation -lc++

BUILD   := build
BIN     := ray

# Engine + new front-end sources (lex.c / parser.c / writeppm.c are gone).
SRCS := \
    src/csg.c src/bound.c src/tri.c src/board.c src/box.c src/conic.c \
    src/sphere.c src/mathfun.c src/raymain.c src/view.c src/init.c \
    src/symtab.c src/attrib.c src/check.c src/range.c src/paramap.c \
    src/mandel.c src/bitmap.c src/global.c src/support.c src/translat.c \
    src/parse_tr.c src/sail.c src/image.c src/jsonscene.c src/bvh.c src/main.c \
    third_party/cJSON.c

OBJS     := $(patsubst %.c,$(BUILD)/%.o,$(SRCS))
METAL_OBJ := $(BUILD)/src/metal_backend.o

.PHONY: all release run bench clean

all: $(BIN)

# Optimized build (see BENCHMARK.md). ~1.3x over the default -O2 baseline,
# single-threaded. Note: -ffast-math relaxes IEEE semantics; verify output.
release: clean
	$(MAKE) CFLAGS="-O3 -ffast-math -flto -mcpu=apple-m4 -fomit-frame-pointer -DNDEBUG -Isrc -Ithird_party $(WARN)" LDFLAGS="-lm -flto -framework Metal -framework Foundation -lc++"

$(BIN): $(OBJS) $(METAL_OBJ)
	$(CC) $(OBJS) $(METAL_OBJ) $(LDFLAGS) -o $(BIN)
	@echo "Built ./$(BIN)"

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Objective-C Metal backend (separate rule: needs -ObjC flag)
$(BUILD)/src/metal_backend.o: src/metal_backend.m
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -ObjC -c $< -o $@

run: $(BIN)
	./$(BIN) scenes/cone.json -o cone.png

# Render the benchmark scene 5x and print render times (see BENCHMARK.md).
bench: $(BIN)
	@for i in 1 2 3 4 5; do ./$(BIN) scenes/benchmark.json -o /tmp/bench.png | grep "render time"; done

# GPU benchmark on the spheres scene (sphere-only; Metal-compatible)
bench-gpu: $(BIN)
	@for i in 1 2 3; do ./$(BIN) scenes/spheres.json --gpu -o /tmp/bench_gpu.png | grep "render time"; done

clean:
	rm -rf $(BUILD) $(BIN)
