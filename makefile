BUILD_DIR ?= build
SOURCEDIR ?= src
LLVM_DIR ?= $(shell brew --prefix llvm)

SOURCES := $(shell find $(SOURCEDIR) -name '*.c') $(shell find $(SOURCEDIR) -name '*.cpp')
OBJECTS_1 := $(SOURCES:%.c=%.o)
OBJECTS := $(OBJECTS_1:%.cpp=%.o)
OUTPUTS := $(OBJECTS:src%=build%)
MY_CC ?= clang++

toy: $(BUILD_DIR)/toy.yy.cpp $(BUILD_DIR)/toy.tab.cpp $(OUTPUTS)
	$(MY_CC) -O3 -Ibdwgc/include -I$(shell llvm-config --includedir) -I$(SOURCEDIR) -I$(BUILD_DIR) $^ bdwgc/libgc.a $(shell llvm-config --cxxflags --ldflags --system-libs --libs core orcjit native) -o $@

$(BUILD_DIR)/%.yy.cpp: src/%.l | $(BUILD_DIR)
	flex -o $@ $<

$(BUILD_DIR)/%.tab.cpp: src/%.y | $(BUILD_DIR)
	bison -Wcounterexamples -d -o $@ $^

$(BUILD_DIR)/%.o: src/%.cpp | $(BUILD_DIR)
	$(MY_CC) -O3 -Ibdwgc/include -I$(shell llvm-config --includedir) $(shell llvm-config --cxxflags) -I$(SOURCEDIR) -I$(BUILD_DIR) -o $@ $< -c

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

.PHONY: clean

clean:
	rm -rf $(BUILD_DIR)
	rm -rf toy