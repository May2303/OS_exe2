# Recursively finds all subdirectories
SUBDIRS := $(wildcard */)

# Define the default target
all: $(SUBDIRS)

# Rule to build all subdirectories
$(SUBDIRS):
	@$(MAKE) -C $@

# Clean rule to clean all subdirectories
clean:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done

# Phony targets to avoid conflicts with file names
.PHONY: all clean $(SUBDIRS)
