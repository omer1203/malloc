#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

int main()
{
    printf("Benchmark Testing\n");
    clock_t start = clock(); // start the timer 

    void *pointers[1500]; // array of pointers for holding mem
    size_t sizes[] = {44,55,69,124,456,789,1278,2645}; // set pf sozes for the allocations

    int num_sizes = sizeof(sizes) / sizeof(sizes[0]); // the number of size options that we have

    for(int i = 0; i < 1500; i++) // loop through the allocated memory blocks
    {
        size_t size = sizes[i % num_sizes]; // choose a size randomly 
        pointers[i] = malloc(size); // allcoate memory using the calculated size 
        if (i % 5 == 0 && pointers[i]) // for every 5th block 
        {
            free(pointers[i]); // free the memory
            pointers[i] = NULL;
        }
    }
    for (int i = 0; i < 1500; i++)  // free any memory that is still allopcated 
    {
        if (pointers[i]) // if still allocated 
        {
            free(pointers[i]); // free it 
        }
    }
    clock_t finish = clock(); // stop the timer once finished 
    double time = (double)(finish - start) / CLOCKS_PER_SEC; // calculte the time taken 
    printf("End Benchmark: time taken: %f seconds\n", time); // print it
    return 0;


}

