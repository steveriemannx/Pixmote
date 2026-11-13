# Pixmote build system.
#
#   make            build the static library, tests and demo
#   make test       build and run the test suite
#   make run-demo   build and run the headless demo (PPM screenshots)
#   make screenshots  render the demo and convert its PPMs to docs/images/*.png
#   make clean      remove all build artifacts
#
# Cross-compiling: override CXX/CXXFLAGS, e.g.
#   make CXX=arm-none-eabi-g++ CXXFLAGS="-std=c++20 -Os -mcpu=cortex-m4 -mthumb"

CXX      ?= g++
AR       ?= ar
CXXFLAGS ?= -std=c++20 -O2 -g -Wall -Wextra -Wpedantic
CPPFLAGS ?= -Iinclude
BUILD    ?= build

LIB  := $(BUILD)/libpixmote.a
SRCS := $(wildcard src/*.cpp)
OBJS := $(SRCS:src/%.cpp=$(BUILD)/obj/%.o)
DEPS := $(OBJS:.o=.d)

TESTS := $(BUILD)/test_pixmote $(BUILD)/test_no_heap
DEMO  := $(BUILD)/pixmote_demo
IMAGES := docs/images

.PHONY: all lib tests demo test run-demo screenshots clean

all: lib tests demo

lib: $(LIB)

tests: $(TESTS)

demo: $(DEMO)

$(BUILD)/obj:
	@mkdir -p $@

$(BUILD):
	@mkdir -p $@

$(BUILD)/obj/%.o: src/%.cpp | $(BUILD)/obj
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(LIB): $(OBJS)
	$(AR) rcs $@ $^

$(BUILD)/test_%: tests/test_%.cpp $(LIB) | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< $(LIB) -o $@

$(DEMO): examples/demo.cpp $(LIB) | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< $(LIB) -o $@

test: $(TESTS)
	@set -e; for test in $(TESTS); do \
		echo "== $$test"; \
		./$$test; \
	done

run-demo: $(DEMO)
	cd $(BUILD) && ./pixmote_demo demo

# Requires python3 only (no third-party modules).
screenshots: $(DEMO)
	@mkdir -p $(IMAGES)
	cd $(BUILD) && ./pixmote_demo demo
	python3 tools/ppm2png.py --scale 2 $(BUILD)/demo_initial.ppm $(IMAGES)/demo_initial.png
	python3 tools/ppm2png.py --scale 2 $(BUILD)/demo.ppm $(IMAGES)/demo.png
	python3 tools/ppm2png.py --scale 2 $(BUILD)/demo_focus.ppm $(IMAGES)/demo_focus.png

clean:
	rm -rf $(BUILD)

-include $(DEPS)
