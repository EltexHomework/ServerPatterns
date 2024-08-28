DIRS=$(wildcard task*)

all: $(DIRS)

# Call all in dirs makefiles
$(DIRS):
	@$(MAKE) --directory $@

# Call clean in all dirs makefiles
clean:
	@for dir in $(DIRS); do \
		$(MAKE) --directory $$dir clean; \
	done

.PHONY: all clean $(DIRS)
