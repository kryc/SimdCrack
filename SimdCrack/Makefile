TARGET ?= simdcrack

BUILD_DIR ?= build
SRC_DIRS ?= .

SRCS := $(wildcard *.cpp)
OBJS := $(SRCS:.cpp=.o)
DEPS := $(OBJS:.o=.d)  # one dependency file for each source

INC_DIRS := ../SimdHash/SimdHash/
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

CXXFLAGS ?= $(INC_FLAGS) -Wall -Werror -mavx2 -O3 -funroll-loops -std=c++17
LDFLAGS = -pthread

.PHONY: $(TARGET)
$(TARGET): $(OBJS)
	$(CXX) -o $@ $(OBJS) ../SimdHash/SimdHash/simdhash.a $(LDFLAGS)

-include $(DEPS)   # include all dep files in the makefile

# rule to generate a dep file by using the C preprocessor
# (see man cpp for details on the -MM and -MT options)
%.d: %.cpp
	$(CXX) $(CPPFLAGS) $< -MM -MT $(@:.d=.o) >$@

.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET) $(DEPS)

.PHONY: all
all: $(TARGET)