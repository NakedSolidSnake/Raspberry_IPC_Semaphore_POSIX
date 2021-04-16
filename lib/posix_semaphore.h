#ifndef POSIX_SEMAPHORE_H_
#define POSIX_SEMAPHORE_H_

#include <stdbool.h>

typedef struct 
{
    void *handle;
    const char *name;
} POSIX_Semaphore;

bool POSIX_Semaphore_Create(POSIX_Semaphore *semaphore);
bool POSIX_Semaphore_Get(POSIX_Semaphore *semaphore);
bool POSIX_Semaphore_Post(POSIX_Semaphore *semaphore);
bool POSIX_Semaphore_Wait(POSIX_Semaphore *semaphore);
bool POSIX_Semaphore_Cleanup(POSIX_Semaphore *semaphore);

#endif /* POSIX_SEMAPHORE_H_ */
