#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>   


struct sound_seg {
    int16_t *data;
    size_t len;
    struct sharedpart *child;
    size_t numofchild; 
};

struct sharedpart {
    struct sound_seg *track; // track that shares this part building a parent to child relationship
    size_t deststart; // where to start in dest
    size_t srcstart; // where to start in source
    size_t sharelen; // length of shared part
};

//functions i will incorporate
    struct sound_seg* tr_init();
    void tr_destroy(struct sound_seg* track);
    size_t tr_length(struct sound_seg* track);
    void tr_write(struct sound_seg* track, const int16_t* src, size_t pos, size_t len);
    void tr_read(struct sound_seg* track, int16_t* dest, size_t pos, size_t len);
    bool tr_delete_range(struct sound_seg* track, size_t pos, size_t len);
    void wav_load(const char* fname, int16_t* dest);
    void wav_save(const char* fname, const int16_t* src, size_t len);
    char* tr_identify(const struct sound_seg* target, const struct sound_seg* ad);
    void tr_insert( struct sound_seg* src_track, struct sound_seg* dest_track, size_t destpos, size_t srcpos, size_t len);