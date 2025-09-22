#include "editor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

void wav_load(const char *fname, int16_t *dest)
{
    FILE *file = fopen(fname, "rb"); // rb opens the file in binary form? am i right? not sure
    if (file == NULL)
    {
        printf("Empty File");
        return;
    }

    // i used general google searches to brush up on fseek, fopen, ftell
    //  i learned about header via this resource containing details about wav file formats http://soundfile.sapp.org/doc/WaveFormat/

    fseek(file, 40, SEEK_SET); //move to data size within header
    uint32_t dataSize = 0;
    fread(&dataSize, sizeof(uint32_t), 1, file); //read the size of data from header
    fread(dest, sizeof(int16_t), dataSize / sizeof(int16_t), file); //read into dest

    fclose(file);
}

void wav_save(const char *fname, const int16_t *src, size_t len)
{
    FILE *file = fopen(fname, "wb"); // open the file. if file doesnt exist create automatically
    if (file == NULL)                // check if something is wrong
    {
        printf("File could not be opened");
        return;
    }

    // making the header
    // u means unsigned
    // 8 = 1 byte, 16 = 2 bytes, 32 = 4 bytes
    //  i gave the appropriate storage to variables for header according to  https://docs.fileformat.com/audio/wav/

    // all of this is bits
    uint16_t pcm = 1;
    uint16_t mono = 1;
    uint32_t sampRate = 8000;
    uint16_t sampleBits = 16;
    uint16_t sampByte = sampleBits / 8; // 2bytes

    // mono * pcm = sample for channel(in this case mono) at a particular time. or frame size
    uint16_t frameSize = mono * sampByte;

    uint8_t header[44] = {0};

    // riff. ascii equivalent will go in the header for the chars
    // also, char is also 1 byte so uint16 works perfectly
    header[0] = 'R';
    header[1] = 'I';
    header[2] = 'F';
    header[3] = 'F';

    // chunksize work
    uint32_t datasize = len * sampByte; // length multiplied by sample each byte gives datasize
    uint32_t chunksize = 36 + datasize; // chunk size tells the size of everything after it. according to the link in spec, should be 36 + datasize for pcm

    union chunk
    {
        uint32_t val;
        uint8_t bytes[4];
    };

    union chunk chunkU;
    chunkU.val = chunksize; // step with the union means since val and bytes share same memory, i can pass my value to val and use my array which is 8 bits per element (total 32) to represent it as 4 separate bytes in my array. this is type punning and i got it from here https://stackoverflow.com/questions/44137442/what-is-type-punning-and-what-is-the-purpose-of-it#:~:text=30-,Type%20punning,data%20accessed%20through%20both%20pointers.)

    header[4] = chunkU.bytes[0];
    header[5] = chunkU.bytes[1];
    header[6] = chunkU.bytes[2];
    header[7] = chunkU.bytes[3];

    // wave and fmt. same logic as riff
    header[8] = 'W';
    header[9] = 'A';
    header[10] = 'V';
    header[11] = 'E';

    header[12] = 'f';
    header[13] = 'm';
    header[14] = 't';
    header[15] = ' ';

    //same union idea for fmt
    union chunk fmtSize;
    fmtSize.val = 16; // per https://www.mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html
    for (int i = 16; i <= 19; i++)
    {
        header[i] = fmtSize.bytes[i-16];
    }
    
    //storing format size
    union format {
        uint16_t val; //bcs 2 bytes so uint16
        uint8_t bytes[2];
    };

    union format formSize;
    formSize.val = pcm;

    for (int i = 20; i <= 21; i++)
    {
        header[i] = formSize.bytes[i-20];
    }

    //storing mono using same union as format
    union format monoUnion;
    monoUnion.val = mono; 
    for (int i = 22; i <= 23; i++)
    {
        header[i] = monoUnion.bytes[i-22];
    }   
    

    //storing sample rate
    union sample
    {
        uint32_t value;
        uint8_t bytes[4];
    };

    union sample sampling;
    sampling.value = sampRate;
    for (int i = 24; i <= 27; i++)
    {
        header[i] = sampling.bytes[i-24];
    } 

    //storing byte rate
    union byteRate 
    {
        uint32_t val;
        uint8_t bytes[4];
    };

    union byteRate byteR;
    byteR.val = sampRate * frameSize;
    for (int i = 28; i <= 31; i++)
    {
        header[i] = byteR.bytes[i-28];
    } 

    //storing frame size
    union frames {
        uint16_t val;
        uint8_t bytes[2];
    };

    union frames fSize;
    fSize.val = frameSize;
    for (int i = 32; i <= 33; i++)
    {
        header[i] = fSize.bytes[i-32];
    } 

    //storing sample bits
    union bitpersamp {
        int16_t val;
        int8_t bytes[2];
    };

    union bitpersamp bitsam;
    bitsam.val = sampleBits;
    for (int i = 34; i <= 35; i++)
    {
        header[i] = bitsam.bytes[i-34];
    } 

    //storing "data"
    header[36] = 'd';
    header [37] = 'a';
    header [38] = 't';
    header[39] = 'a';

    //finally data size
    union mydata {
        uint32_t val;
        uint8_t bytes[4];
    };

    union mydata data;
    data.val = datasize;
    for (int i = 40; i <= 43; i++)
    {
        header[i] = data.bytes[i-40];
    } 
    
    // write the header onto the file
    fwrite(header, sizeof(int8_t), 44, file);

    //write the actual data now
    fwrite(src, sizeof(int16_t), len, file);

    //now close the file
    fclose(file);
    
}