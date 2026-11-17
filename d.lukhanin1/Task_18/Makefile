CC = cc
CPPFLAGS += -D_POSIX_C_SOURCE=200809L -D_FILE_OFFSET_BITS=64
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2

all: file_listing

file_listing: file_listing.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDLIBS)

check: all
	python3 ../test_listing_signal.py ListingTests

clean:
	rm -f file_listing

.PHONY: all check clean
