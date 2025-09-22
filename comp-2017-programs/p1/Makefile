#TODO
# compile and flags
CC = gcc
CFLAGS = -Wall -Wextra -fPIC  # fPIC is required for shared libraries according to spec

# Build sound_seg.o
sound_seg.o: track.o wav.o identify.o
	$(CC) -r track.o wav.o identify.o -o sound_seg.o

# Compile each source file into its object file
track.o: track.c editor.h
	$(CC) $(CFLAGS) -c track.c -o track.o

wav.o: wav.c editor.h
	$(CC) $(CFLAGS) -c wav.c -o wav.o

identify.o: identify.c editor.h
	$(CC) $(CFLAGS) -c identify.c -o identify.o

# Clean up object files
clean:
	rm -f track.o wav.o identify.o sound_seg.o