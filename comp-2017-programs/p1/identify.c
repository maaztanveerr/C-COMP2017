#include "editor.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

char* tr_identify(const struct sound_seg* target, const struct sound_seg* ad) {
    //get the length of both track and ad
    size_t targLen = (*target).len;
    size_t adLen = (*ad).len; 

    //check if ad longer than target
    if (adLen > targLen)
    {
        printf("Ad greater than target");
        return NULL;
    }
    
    // autocorelation first
    //also using int64  to prevent overflow as bigger numbers involving constant multiplication 
    int64_t autorel = 0;
    for (size_t i = 0; i < adLen; i++)
    {
        autorel += (int64_t) (*ad).data[i] * (*ad).data[i]; //our 100% match
    }

    int64_t max = (int64_t) (0.95 * (double) autorel); //our max similarity set at 95%
    size_t pos = targLen - adLen + 1;

    int64_t *crossrel = malloc(pos * sizeof(int64_t)); //assign memory to store an array for cross corelation
    if (crossrel == NULL) { //check if malloc worked
        printf("Memory could not be allocated for crossrel");
        return NULL;
    }
    
    //calculate crosscorelation for each
    for (size_t i = 0; i < pos; i++)
    {
        int64_t sum = 0; // current dot product sum
        for (size_t j = 0; j < adLen; j++)
        {
            sum += (int64_t) (*target).data[i + j] * (*ad).data[j]; // calculate new dot product by multiplying ad element with corresponding target element and then adding with other positions
        }
        crossrel[i] = sum; 
    }

    size_t storage = 1024; // guessed a large number to store string. doesnt matter that much as i can use realloc later
    size_t current = 0; //to track current place
    int charcount = 0; //return value for snprintf | num of chars to be written

    //allocate memory for result
    char *result = malloc(storage);
    if (result == NULL) { //check again
        printf("Memory could not be allocated for result");
        free(crossrel);
        return NULL;
    }

    result[0] = '\0'; //initialise as empty first
    bool firstmatch = true; //to fix newline errors

    //loop over every possible position in target where ad can fit to check for matches
    for (size_t i = 0; i < pos; i++)
    {
        if (crossrel[i] >= max) //if match (95% or above)
        {   
            size_t end = i + adLen -1;
            if (firstmatch == false)
            {
                if (current >= storage - 2) {
                    storage = storage * 2;
                    char* tempr = realloc(result, storage);
                    if (tempr == NULL) {
                        free(crossrel);
                        free(result);
                        return NULL;
                    }
                    result = tempr;
                }
                result[current] = '\n';
                current++;
                result[current] = '\0';
            } else {
                firstmatch = false;
            }

            // used this to learn about int to str using snprintf https://stackoverflow.com/questions/8257714/how-can-i-convert-an-int-to-a-string-in-c 

            //also learnt about %zu from https://www.tutorialspoint.com/what-is-the-correct-way-to-use-printf-to-print-a-size-t-in-c-cplusplus#:~:text=We%20should%20use%20%E2%80%9C%25zu%E2%80%9D,use%20of%20%E2%80%9C%25zu%E2%80%9D. 

            charcount = snprintf((result + current), storage - current, "%zu,%zu", i, end ); // ad has len samples and we move from i, i+1,.., i + len -1 which is what i call variable end

            //below ensures that incase snprintf runs out of memory we reallocate memory for result to allow chars to be written
            if (charcount < 0 || current + charcount >= storage - 1) //storage is character which is 1 byte so comparasion possible to check size
            {
                storage = storage * 2;
                char* temp = realloc(result, storage);
                if (temp == NULL) {
                    printf("Reallocation failed. tridentify");
                    free(crossrel);
                    free(result); 
                    return NULL;
                }
                result = temp;

                // got more memory now so try again
                charcount = snprintf(result + current, storage - current, "%zu,%zu", i, end);
            }

            current = current + charcount; // move ahead of the chars written justg now

            i = i + (*ad).len -1; //skip to avoid same matches again and again

        }
        
        
    }

    free(crossrel);
    result[current] = '\0'; 
    
    return result;
}