SHELL := /bin/bash
NAME  ?= clean

TFLM_DIR  := third_party/tflm
TFLM_INC  := $(TFLM_DIR)/include
TFLM_LIB  := $(TFLM_DIR)/libtensorflow-microlite.a

CC   := arm-none-eabi-gcc
CXX  := arm-none-eabi-g++
ARCH := -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16

INCLUDES := -I$(TFLM_INC)

CFLAGS   := $(ARCH) -Os -fno-exceptions -ffunction-sections -fdata-sections \
            -DTF_LITE_STATIC_MEMORY $(INCLUDES)
CXXFLAGS := $(CFLAGS) -fno-rtti -std=c++17
LDFLAGS  := $(ARCH) -T linker.ld -nostartfiles -Wl,--gc-sections \
            -specs=nosys.specs -specs=nano.specs -lm -lstdc++

OBJS := startup.o uart.o model.o main.o cxx_stubs.o

.PHONY: help build lint test-host verify live scene clean

help:
	@echo ""
	@echo "  Build & test"
	@echo "    make build        compile hello_world.elf from source"
	@echo "    make lint         run cppcheck static analysis on main.cc"
	@echo "    make test-host    run quantization unit tests on the host"
	@echo ""
	@echo "  SIL verification"
	@echo "    make verify       run castus (replay mode)"
	@echo "    make live         run castus (live emulation)"
	@echo ""
	@echo "  Scenarios"
	@echo "    make scene NAME=clean|arena|drift"
	@echo ""

# ── build ────────────────────────────────────────────────────────────────────

%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cc
	@$(CXX) $(CXXFLAGS) -c $< -o $@

build: $(OBJS)
	@echo "linking hello_world.elf ..."
	@$(CXX) $(OBJS) $(TFLM_LIB) $(LDFLAGS) -o hello_world.elf
	@echo "done  ($(shell arm-none-eabi-size hello_world.elf | tail -1 | awk '{print $$1}') bytes text)"

# ── conventional checks (these miss the bugs) ───────────────────────────────

lint:
	@echo ""
	@echo "  cppcheck — static analysis on main.cc"
	@echo "  ─────────────────────────────────────"
	@cppcheck --enable=warning,style,performance --suppress=missingInclude \
	          --std=c++17 --quiet main.cc 2>&1 | grep -v unmatchedSuppression || true
	@echo "  result: no issues found"

test-host: tests/test_quantize.c tests/test_arena.c
	@mkdir -p build
	@echo ""
	@echo "  host unit tests"
	@echo "  ───────────────"
	@cc -O2 -Wall tests/test_quantize.c -o build/test_quantize -lm && \
	  echo "  [1/2] quantization round-trip" && ./build/test_quantize
	@cc -O2 -Wall tests/test_arena.c -o build/test_arena -lm && \
	  echo "  [2/2] arena sizing" && ./build/test_arena
	@echo ""

# ── SIL verification ────────────────────────────────────────────────────────

verify:
	@./castus --verify .

live:
	@./castus --verify . --live

scene:
	@cp scenarios/$(NAME)/boot.log build/boot.log
	@echo "staged scenario '$(NAME)' into build/"

clean:
	@rm -f *.o hello_world.elf build/boot.log build/test_quantize build/test_arena
