#include <stdbool.h>
#include <stdlib.h> //memory allocation again
#include "editor.h" // my header
#include <stdio.h>
#include <stdint.h>
#include <string.h>

struct sound_seg* tr_init() {
    //memory allocation and check
    struct sound_seg *track = malloc(sizeof(struct sound_seg));
    if (track == NULL)
    {
        printf("Allocation failed");
        return NULL;
    }

    //initialize
    (*track).data = NULL;
    (*track).len = 0;
    (*track).child = NULL;
    (*track).numofchild = 0;

    return track; 
}

void tr_destroy(struct sound_seg *track) {
    if (track == NULL) //check
    {
        printf("Pointer already Null. No need to free");
        return;
    }
    
    free((*track).data); //freeing pointer inside struct
    free((*track).child); //freeing the child
    free(track); // freeing struct itself
}

size_t tr_length(struct sound_seg *track) {
    if (track == NULL) //check
    {
        printf("Cannot determine Length. Pointer is null");
        return 0;
    }

    return (*track).len; //return length
}

void tr_write(struct sound_seg *track, const int16_t *src, size_t pos, size_t len) {

    //performing checks
    if (track == NULL || src == NULL)
    {
        printf("Could not write due to NULL values");
        return;
    }

    size_t requriedSize = pos + len; 
    int16_t *newData = 0;

    if (requriedSize > (*track).len)
    {
        newData = realloc((*track).data, requriedSize * sizeof(int16_t));
        if (newData == NULL)
        {
            printf("Memory could not be altered"); //hopefully does not happen :)
            return;
        }

        (*track).data = newData;
        (*track).len = requriedSize;        //not sure if i did this right. self reminder to check later!!!
        
    }
    
    for (size_t i = 0; i < len; i++)
    {
        (*track).data[pos+i] = src[i];  //actually write the data
    }


    // to get part 3 to work:
    // writing to parent should mean that if portion is shared with child, child should also get the changes
    //therefore this:

    for (size_t i = 0; i < (*track).numofchild; i++) //loop through children of the current track
    {
        struct sharedpart *currchild = &((*track).child[i]); //get current child, which shares data with parent, to write to
        
        size_t pstart = (*currchild).srcstart; // point at which shared child data starts in parent (i.e src)
        size_t pend = (*currchild).sharelen + pstart; //end point in parent track of shared part


        // basically checking if write is happening in the shared portion
        size_t sharedWriteStart = 0;
        size_t sharedWriteEnd = 0;  // these two variables are for the write operation keeping track of the start and end of shared data which is affected by our write operation
        
        // size_t sharedWriteLen = 0; // length of shared part in write
        if ((pos < pend) && ((pos + len) > pstart))
        { //these two are checks ensuring that parent-child write process only happens if trwrite writes to the parent in the section it shares with child
    
                if (pos > pstart)
                {
                    sharedWriteStart = pos;
                } else {
                    sharedWriteStart = pstart;
                }
                 
                if ((pos + len) < pend)
                {
                    sharedWriteEnd = pos+ len;
                } else {
                    sharedWriteEnd = pend;
                }
                
         }
        

        if (sharedWriteStart < sharedWriteEnd)
        {
            // parentStartgap means that im getting where the shared data begins in parent to which im writing to meaning how far it is from the start
            size_t parentStartgap = sharedWriteStart - pstart;
            // and then essentially i just add it to the child to write to the correct position in child for shared data        // i struggled so much with this  :/
            size_t childWrite = (*currchild).deststart +parentStartgap; 
            size_t sharedWriteLen = sharedWriteEnd - sharedWriteStart;
            size_t srcSharePoint = sharedWriteStart - pos; //it is the positon to start copying data from into child //  index for data supposed to be shared between parent and child.

            // If child.track == parent track, do a single memmove within the same array
            if ((*currchild).track == track)
            {
                
                memmove(&(*track).data[childWrite], &(*track).data[sharedWriteStart], sharedWriteLen * sizeof(int16_t));
            } else { //copy the shared samples from parent to child
                struct sound_seg *childtrack = currchild->track;

                for (size_t i = 0; i < sharedWriteLen; i++)
                {
                    childtrack->data[childWrite + i] = src[srcSharePoint + i];
                }
            }   
            
        }
        
        
    }
    
}

void tr_read(struct sound_seg *track, int16_t *dest, size_t pos, size_t len) {
    if (track == NULL || dest == NULL || pos + len > (*track).len) //check
    {
        printf("Read Failed. ");
        return;
    }

    for (size_t i = 0; i < len; i++)
    {
        dest[i] = (*track).data[pos+ i];  //actually copy here
    }
}

bool tr_delete_range(struct sound_seg *track, size_t pos, size_t len) {
    if (track == NULL)
    {
        printf("Delete Failed. Null Track");
        return false;
    }

    //basically dont delete if child present
    for (size_t i = 0; i < track->numofchild; i++)
    {
        size_t sharestart = track->child[i].srcstart;
        size_t shareEnd = sharestart + track->child[i].sharelen;

        size_t deletestart = pos;
        size_t deleteEnd = pos + len;

        if (deletestart < shareEnd)
        {
            if (deleteEnd > sharestart)
            {
                printf("Delete failed as data is shared with child");
                return false;
            }
            
        }
        
    }
    
    for (size_t i = pos + len; i < (*track).len; i++)
    {
        (*track).data[i - len] = (*track).data[i];  //shift last values to pos
    }

    (*track).len = (*track).len - len; //delete the last values
    
    return true;
}

// void wav_load(const char *fname, int16_t *dest)
// {
//     FILE *file = fopen(fname, "rb"); // rb opens the file in binary form? am i right? not sure
//     if (file == NULL)
//     {
//         printf("Empty File");
//         return;
//     }


//     // i used general google searches to brush up on fseek, fopen, ftell
//     //  i learned about header via this resource containing details about wav file formats http://soundfile.sapp.org/doc/WaveFormat/

//     fseek(file, 40, SEEK_SET); //move to data size within header
//     uint32_t dataSize = 0;
//     fread(&dataSize, sizeof(uint32_t), 1, file); //read the size of data from header
//     fread(dest, sizeof(int16_t), dataSize / sizeof(int16_t), file); //read into dest

//     fclose(file);
// }

// void wav_save(const char *fname, const int16_t *src, size_t len)
// {
//     FILE *file = fopen(fname, "wb"); // open the file. if file doesnt exist create automatically
//     if (file == NULL)                // check if something is wrong
//     {
//         printf("File could not be opened");
//         return;
//     }

//     // making the header
//     // u means unsigned
//     // 8 = 1 byte, 16 = 2 bytes, 32 = 4 bytes
//     //  i gave the appropriate storage to variables for header according to  https://docs.fileformat.com/audio/wav/

//     // all of this is bits
//     uint16_t pcm = 1;
//     uint16_t mono = 1;
//     uint32_t sampRate = 8000;
//     uint16_t sampleBits = 16;
//     uint16_t sampByte = sampleBits / 8; // 2bytes

//     // mono * pcm = sample for channel(in this case mono) at a particular time. or frame size
//     uint16_t frameSize = mono * sampByte;

//     uint8_t header[44] = {0};

//     // riff. ascii equivalent will go in the header for the chars
//     // also, char is also 1 byte so uint16 works perfectly
//     header[0] = 'R';
//     header[1] = 'I';
//     header[2] = 'F';
//     header[3] = 'F';

//     // chunksize work
//     uint32_t datasize = len * sampByte; // length multiplied by sample each byte gives datasize
//     uint32_t chunksize = 36 + datasize; // chunk size tells the size of everything after it. according to the link in spec, should be 36 + datasize for pcm

//     union chunk
//     {
//         uint32_t val;
//         uint8_t bytes[4];
//     };

//     union chunk chunkU;
//     chunkU.val = chunksize; // step with the union means since val and bytes share same memory, i can pass my value to val and use my array which is 8 bits per element (total 32) to represent it as 4 separate bytes in my array. this is type punning and i got it from here https://stackoverflow.com/questions/44137442/what-is-type-punning-and-what-is-the-purpose-of-it#:~:text=30-,Type%20punning,data%20accessed%20through%20both%20pointers.)

//     header[4] = chunkU.bytes[0];
//     header[5] = chunkU.bytes[1];
//     header[6] = chunkU.bytes[2];
//     header[7] = chunkU.bytes[3];

//     // wave and fmt. same logic as riff
//     header[8] = 'W';
//     header[9] = 'A';
//     header[10] = 'V';
//     header[11] = 'E';

//     header[12] = 'f';
//     header[13] = 'm';
//     header[14] = 't';
//     header[15] = ' ';

//     //same union idea for fmt
//     union chunk fmtSize;
//     fmtSize.val = 16; // per https://www.mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html
//     for (int i = 16; i <= 19; i++)
//     {
//         header[i] = fmtSize.bytes[i-16];
//     }
    
//     //storing format size
//     union format {
//         uint16_t val; //bcs 2 bytes so uint16
//         uint8_t bytes[2];
//     };

//     union format formSize;
//     formSize.val = pcm;

//     for (int i = 20; i <= 21; i++)
//     {
//         header[i] = formSize.bytes[i-20];
//     }

//     //storing mono using same union as format
//     union format monoUnion;
//     monoUnion.val = mono; 
//     for (int i = 22; i <= 23; i++)
//     {
//         header[i] = monoUnion.bytes[i-22];
//     }   
    

//     //storing sample rate
//     union sample
//     {
//         uint32_t value;
//         uint8_t bytes[4];
//     };

//     union sample sampling;
//     sampling.value = sampRate;
//     for (int i = 24; i <= 27; i++)
//     {
//         header[i] = sampling.bytes[i-24];
//     } 

//     //storing byte rate
//     union byteRate 
//     {
//         uint32_t val;
//         uint8_t bytes[4];
//     };

//     union byteRate byteR;
//     byteR.val = sampRate * frameSize;
//     for (int i = 28; i <= 31; i++)
//     {
//         header[i] = byteR.bytes[i-28];
//     } 

//     //storing frame size
//     union frames {
//         uint16_t val;
//         uint8_t bytes[2];
//     };

//     union frames fSize;
//     fSize.val = frameSize;
//     for (int i = 32; i <= 33; i++)
//     {
//         header[i] = fSize.bytes[i-32];
//     } 

//     //storing sample bits
//     union bitpersamp {
//         int16_t val;
//         int8_t bytes[2];
//     };

//     union bitpersamp bitsam;
//     bitsam.val = sampleBits;
//     for (int i = 34; i <= 35; i++)
//     {
//         header[i] = bitsam.bytes[i-34];
//     } 

//     //storing "data"
//     header[36] = 'd';
//     header [37] = 'a';
//     header [38] = 't';
//     header[39] = 'a';

//     //finally data size
//     union mydata {
//         uint32_t val;
//         uint8_t bytes[4];
//     };

//     union mydata data;
//     data.val = datasize;
//     for (int i = 40; i <= 43; i++)
//     {
//         header[i] = data.bytes[i-40];
//     } 
    
//     // write the header onto the file
//     fwrite(header, sizeof(int8_t), 44, file);

//     //write the actual data now
//     fwrite(src, sizeof(int16_t), len, file);

//     //now close the file
//     fclose(file);
    
// }


// -------------------------- Part 2 -------------------------- //


// char* tr_identify(const struct sound_seg* target, const struct sound_seg* ad) {
//     //get the length of both track and ad
//     size_t targLen = (*target).len;
//     size_t adLen = (*ad).len; 

//     //check if ad longer than target
//     if (adLen > targLen)
//     {
//         printf("Ad greater than target");
//         return NULL;
//     }
    
//     // autocorelation first
//     //also using int64  to prevent overflow as bigger numbers involving constant multiplication 
//     int64_t autorel = 0;
//     for (size_t i = 0; i < adLen; i++)
//     {
//         autorel += (int64_t) (*ad).data[i] * (*ad).data[i]; //our 100% match
//     }

//     int64_t max = (int64_t) (0.95 * (double) autorel); //our max similarity set at 95%
//     size_t pos = targLen - adLen + 1;

//     int64_t *crossrel = malloc(pos * sizeof(int64_t)); //assign memory to store an array for cross corelation
//     if (crossrel == NULL) { //check if malloc worked
//         printf("Memory could not be allocated for crossrel");
//         return NULL;
//     }
    
//     //calculate crosscorelation for each
//     for (size_t i = 0; i < pos; i++)
//     {
//         int64_t sum = 0; // current dot product sum
//         for (size_t j = 0; j < adLen; j++)
//         {
//             sum += (int64_t) (*target).data[i + j] * (*ad).data[j]; // calculate new dot product by multiplying ad element with corresponding target element and then adding with other positions
//         }
//         crossrel[i] = sum; 
//     }

//     size_t storage = 1024; // guessed a large number to store string. doesnt matter that much as i can use realloc later
//     size_t current = 0; //to track current place
//     int charcount = 0; //return value for snprintf | num of chars to be written

//     //allocate memory for result
//     char *result = malloc(storage);
//     if (result == NULL) { //check again
//         printf("Memory could not be allocated for result");
//         free(crossrel);
//         return NULL;
//     }

//     result[0] = '\0'; //initialise as empty first
//     bool firstmatch = true; //to fix newline errors

//     //loop over every possible position in target where ad can fit to check for matches
//     for (size_t i = 0; i < pos; i++)
//     {
//         if (crossrel[i] >= max) //if match (95% or above)
//         {   
//             size_t end = i + adLen -1;
//             if (firstmatch == false)
//             {
//                 if (current >= storage - 2) {
//                     storage = storage * 2;
//                     char* tempr = realloc(result, storage);
//                     if (tempr == NULL) {
//                         free(crossrel);
//                         free(result);
//                         return NULL;
//                     }
//                     result = tempr;
//                 }
//                 result[current] = '\n';
//                 current++;
//                 result[current] = '\0';
//             } else {
//                 firstmatch = false;
//             }

//             // used this to learn about int to str using snprintf https://stackoverflow.com/questions/8257714/how-can-i-convert-an-int-to-a-string-in-c 

//             //also learnt about %zu from https://www.tutorialspoint.com/what-is-the-correct-way-to-use-printf-to-print-a-size-t-in-c-cplusplus#:~:text=We%20should%20use%20%E2%80%9C%25zu%E2%80%9D,use%20of%20%E2%80%9C%25zu%E2%80%9D. 

//             charcount = snprintf((result + current), storage - current, "%zu,%zu", i, end ); // ad has len samples and we move from i, i+1,.., i + len -1 which is what i call variable end

//             //below ensures that incase snprintf runs out of memory we reallocate memory for result to allow chars to be written
//             if (charcount < 0 || current + charcount >= storage - 1) //storage is character which is 1 byte so comparasion possible to check size
//             {
//                 storage = storage * 2;
//                 char* temp = realloc(result, storage);
//                 if (temp == NULL) {
//                     printf("Reallocation failed. tridentify");
//                     free(crossrel);
//                     free(result); 
//                     return NULL;
//                 }
//                 result = temp;

//                 // got more memory now so try again
//                 charcount = snprintf(result + current, storage - current, "%zu,%zu", i, end);
//             }

//             current = current + charcount; // move ahead of the chars written justg now

//             i = i + (*ad).len -1; //skip to avoid same matches again and again

//         }
        
        
//     }

//     free(crossrel);
//     result[current] = '\0'; 
    
//     return result;
// }


//-------------------- Part 3 --------------------
// i looked it up on ed and my code was having problem because of the const for srctrack
// according to thread 1288 the response was that this const can be removed
// so i removed it and life is better :)
void tr_insert( struct sound_seg* src_track, struct sound_seg* dest_track, size_t destpos, size_t srcpos, size_t len) {
        //basic checks for input
        if (srcpos >= (*src_track).len  || destpos > (*dest_track).len || srcpos + len > (*src_track).len)
        {
            printf("source track exceeded");
            return;
        }
        
        size_t oldlen = (*dest_track).len; //old length of track
        size_t trlen = (*dest_track).len + len; // new length for track after insertion

        int16_t *trdata = realloc((*dest_track).data, sizeof(int16_t) * trlen); // change memory allocation for new data to be stored
        if (trdata == NULL) //check if allocated
        {
            printf("Memory could not be allocated ");
            return;
        }
        (*dest_track).data = trdata; //have dest data point to the new memory

        //in test case for trinsert sometimes inputs are same i.e s0, s0, 32, 32,4 so for s0 and s0 im doing this step to resolve my error
        int16_t *temp = NULL;
        if (src_track == dest_track)
        {
            temp = malloc(sizeof(int16_t) * len); // allocating memory for a temp
            if (temp == NULL)
            {
                printf("Memory could not be allocated for temp"); //check
                return;
            }

            // copy data to temp. doesnt matter if source or dest data since theyre the same
            for (size_t i = 0; i < len; i++)
            {
                temp[i] = (*src_track).data[srcpos + i];
            }
        }

        //shift dest data to right
        for (int i = (int) oldlen - 1; i >= (int) destpos; i--)
        {
            (*dest_track).data[i+len] = (*dest_track).data[i];
        }

        if (src_track == dest_track)
        {
            // copy temp into destination
            for (size_t i = 0; i < len; i++)
            {
                (*dest_track).data[destpos +i] = temp[i];
            }
            
            free(temp);

        } else {

            // copy from source to dest
            for (size_t i = 0; i < len; i++)
            {
                (*dest_track).data[destpos+i] = (*src_track).data[srcpos + i];
            }
            
        }
        // change length
        (*dest_track).len = trlen;

        //linked list implementation coming up

        //prereqs for realloc to make it cleaner
        size_t newChildNum = (*src_track).numofchild +1; //since trinsert is called so new child added
        size_t newSize = newChildNum * sizeof(struct sharedpart);
        
        struct sharedpart *tempm = realloc((*src_track).child, newSize);  //reallocate memory to account for new child
        if (tempm == NULL)
        {
            printf("memory not allocated. debug for child at trinsert (linkedlist)");
            return;
        }
        (*src_track).child = tempm; 

        struct sharedpart *newChild = &((*src_track).child[newChildNum - 1]);
        (*newChild).track = dest_track;
        (*newChild).deststart = destpos;
        (*newChild).srcstart = srcpos;
        (newChild)->sharelen = len; //i just realized that i can use the arrow notation and life will be simpler. sorry about the earlier formatting
        src_track->numofchild = newChildNum;

}


// //in test case for trinsert sometimes inputs are same i.e s0, s0, 32, 32,4 so for s0 and s0 im utilizing a temp
        // if (src_track == dest_track)
        // {
        //     //copy data in now
        //     // using a temp to store the data that needs to be copied
        //     int16_t *temp = malloc(len * sizeof(int16_t));
        //     if (temp == NULL)
        //     {
        //         printf("Memory could not be allocated for temp"); //check
        //         return;
        //     }
        //     memcpy(temp, (*dest_track).data + srcpos, len * sizeof(int16_t));

        //     // shift old data using memmove to the right
        //     // memmove is safer for overlapping regions.
        //     memmove((*dest_track).data + destpos + len, (*dest_track).data + destpos, (oldlen - destpos) * sizeof(int16_t));

        //     // memcpy(temp, (*src_track).data + srcpos, len * sizeof(int16_t)); //copy into temp first
        //     memcpy((*dest_track).data + destpos, temp, len * sizeof(int16_t) );
        //     free(temp);
        // } else {
        //     memmove((*dest_track).data + destpos + len, (*dest_track).data + destpos, (oldlen - destpos) * sizeof(int16_t));
        //     memcpy((*dest_track).data + destpos, (*src_track).data +srcpos, len * sizeof(int16_t));
        // }


        