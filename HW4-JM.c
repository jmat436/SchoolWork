/*
    Author: Jamie Mattern
    Email: jmattern1@live.esu.edu
    Date: 03/05/2024
    Purpose: Parse through a very large .txt file using 16 threads and tally the alphanumerical characters
*/

// Include statements
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <regex.h>


// 16 Threads are to be used for this assignment
#define NUM_THREADS 16

// A mutex will be used to lock and unlock the wordCount variable as needed
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int wordCount = 0;

/* 
    threadInfo struct to hold all necessary info for the threads:
    1) int start: starting index for the read
    2) int end: ending index for the read
    3) char* file: file name 
*/
struct threadInfo {
    int start;
    int end;
    char* file;
};

// parses through a segment of a file based on its passed arg threadInfo struct and counts all words in the segment 
void *parseFile (void *arg)
{
    // Get thread info from the passed argument
    struct threadInfo *data = (struct threadInfo *)arg;

    // Open the file for reading
    FILE *file = fopen(data->file, "r");

    // Handle any errors that may arise while opening the file
    if (file == NULL) 
    {
        perror("Failed to open the file");
        pthread_exit(NULL);
    }

    // Initialize variables for counting words and tracking characters
    int count = 0;
    int characters;
    int word = 0;
    int prevCharacter = ' ';

    // Lock the mutex to ensure thread safety
    pthread_mutex_lock(&mutex);

    // Move the file pointer to the specified starting position
    fseek(file, data->start, SEEK_SET);

    // Iterate through the file content withing the specified range
    while ((characters = fgetc(file)) != EOF && ftell(file) <= data->end) {
        // Check for whitespace characters (space, newline, tab)
        if (characters == ' ' || characters == '\n' || characters == '\t') {
            if (word) {
                // Check if the previous character was not whitespace
                if (prevCharacter != ' ' && prevCharacter != '\n' && prevCharacter != '\t') {
                    // Increae the count if it was not
                    count++;
                 }
                word = 0;   // Reset word bool
            }
        } else word = 1;    // Set word flag if a non-whitespace character is encountered
        prevCharacter = characters; // Save the previous character
    } // end while
    
    // Check if the last word extends to the end of the specified range
    if (word && prevCharacter != ' ' && prevCharacter != '\n' && prevCharacter != '\t') 
    {
        count++;
    }

    // Close the file
    fclose(file);

    // Update the global word count
    wordCount += count;

    // Unlock the mutex
    pthread_mutex_unlock(&mutex);

    // Exit the thread
    pthread_exit(NULL);
}

int main()
{
    // open the .txt file and handle any possible errors
    const char* fileName = "Assign4.txt";
    FILE* file = fopen(fileName, "r");
    if (file == NULL) 
    {
        perror("Failed to open the file");
        return 1;
    }

    // Use fseek() to read all bytes in the file, this will be our file size
    fseek(file, 0, SEEK_END);
    int fileSize = ftell(file);
    fclose(file);
    
    // file size is 1796441 bytes
    // 1796441 / 16 = 112277, so thats how many bytes each segment will cover
    // 1796441 % 16 = 9, so the last segment needs to cover an extra 9 bytes

    // declare an array of pthreads and threadInfos for all 16 threads
    pthread_t threads[NUM_THREADS];
    struct threadInfo segmentInfo[NUM_THREADS];
    
    for (int i = 0; i < NUM_THREADS; i++)
    {   
        // set both the start and end index to the respective positions based on fileSize and NUM_THREADS
        segmentInfo[i].start = i * (fileSize / NUM_THREADS);
        segmentInfo[i].end = (i + 1) * (fileSize / NUM_THREADS) - 1;
        segmentInfo[i].file = (char*)fileName;

        // on the last iteration, make the end byte 9 bytes larger to account for the remainder of fileSize % 16
        if (i == 15) segmentInfo[i].end += (fileSize % NUM_THREADS);

        // create the thread
        pthread_create(&threads[i], NULL, parseFile, &segmentInfo[i]);
    }

    // Wait for all threads to finish using pthread_join()
    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    // print wordCount and destroy the mutex
    printf("\nTotal word count in the file: %d\n\n", wordCount);
    
    pthread_mutex_destroy(&mutex);
    return 0;
}

