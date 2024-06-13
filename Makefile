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


# @ in front of the line supres printing the command
# $@ means "solving the variable". in line 9, the make command will
# replace $@ with subdir to execute
