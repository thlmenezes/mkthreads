SOURCE_DIR  := source
OBJECT_DIR  := include

sources     := $(wildcard $(SOURCE_DIR)/*.c)
# Extract file' basename
basenames   := $(basename $(notdir $(sources)))
objects     := $(sources:$(SOURCE_DIR)/%.c=$(OBJECT_DIR)/%.o)
deps        := $(objects:.o=.d)
files       := $(deps) $(basenames) $(objects) $(deps)

CC          := gcc
CPPFLAGS    := -MMD -MP
CFLAGS      := -g -Wall -pedantic
LDLIBS      := -pthread

% : $(OBJECT_DIR)/%.o
	$(LINK.o) $^ $(LDLIBS) -o $@

$(OBJECT_DIR)/%.o : $(SOURCE_DIR)/%.c
	$(COMPILE.c) $(OUTPUT_OPTION) $<

main: $(basenames)

clean:
	@rm $(files)
	@echo 'make clean terminated'

-include $(deps)