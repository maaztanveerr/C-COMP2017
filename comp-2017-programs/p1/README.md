# COMP2017 Project 1 — Audio Editing Library in C

## Overview
This project is an implementation of a lightweight audio editing backend written in C.  
It was developed as part of **COMP2017: Systems Programming** at the University of Sydney.  

The system introduces a `sound_seg` structure to represent audio tracks, along with operations for:
- Initializing and destroying tracks
- Writing and reading audio data
- Inserting and deleting ranges
- Managing shared segments between parent/child tracks
- Loading and saving WAV files
- Identifying snippets within longer tracks using cross-correlation

---
```
## File Structure
├── editor.h     # Header file defining core structs and function prototypes
├── track.c      # Implements track operations (init, destroy, read, write, insert, delete)
├── wav.c        # Implements WAV file load/save functionality
├── identify.c   # Implements cross-correlation snippet identification
├── Makefile     # Build instructions for generating sound_seg.o
```
---

## Example Usage
Below is a minimal example showing how to use the library:

```c
#include "editor.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    struct sound_seg *track = tr_init();

    int16_t samples[5] = {1, 2, 3, 4, 5};
    tr_write(track, samples, 0, 5);

    wav_save("output.wav", track->data, tr_length(track));

    struct sound_seg *snippet = tr_init();
    int16_t snippet_data[3] = {2, 3, 4};
    tr_write(snippet, snippet_data, 0, 3);

    char *matches = tr_identify(track, snippet);
    if (matches) {
        printf("Snippet found at positions: %s\n", matches);
        free(matches);
    }

    tr_destroy(track);
    tr_destroy(snippet);
    return 0;
}
```
---

## Key Features

Track Management: Create, write, read, insert, and delete audio segments.

Shared Backing: Maintain parent-child relationships between tracks for memory efficiency.

WAV I/O: Load and save audio in WAV format.

Snippet Identification: Use cross-correlation to detect occurrences of an audio snippet inside a longer track.

---

```
To build the project, run:

make

This produces the object file:

    sound_seg.o — contains the compiled logic from track.c, wav.c, and identify.c.

To clean up object files:

    make clean
```

---


## Notes

Written in C with standard libraries (stdio.h, stdlib.h, stdint.h, string.h).

Designed for educational use in Systems Programming (memory management, pointer operations, and low-level file I/O).

Not intended as a production audio editor, but demonstrates the core techniques used in building one.
