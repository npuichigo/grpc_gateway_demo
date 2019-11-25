# This makefile does nothing but delegating the actual building to cmake.
SHELL := /bin/bash

all:
	@if [[ -d "build" ]]; then \
		cd build && $(MAKE); \
	else \
		mkdir -p build && cd build && cmake .. && $(MAKE); \
	fi

clean: # This will remove ALL build folders.
	@rm -r build*/
