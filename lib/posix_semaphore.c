#include <posix_semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>

bool POSIX_Semaphore_Create(POSIX_Semaphore *semaphore)
{
    bool status = false;

    do 
    {
        if(!semaphore)
            break;

        semaphore->handle = sem_open(semaphore->name, O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO, 0);
        if(!semaphore->handle)
            break;

        status = true;
    } while(false);

    return status;
}

bool POSIX_Semaphore_Get(POSIX_Semaphore *semaphore)
{
    bool status = false;

    do 
    {
        if(!semaphore)
            break;

        if(sem_post(semaphore->handle) == 0)
            status = true;

        semaphore->handle = sem_open(semaphore->name, O_RDWR);
        if(!semaphore->handle)
            break;

        status = true;
        
    } while(false);

    return status;
}

bool POSIX_Semaphore_Post(POSIX_Semaphore *semaphore)
{
    bool status = false;
    if(semaphore && semaphore->handle)
    {
        if(sem_post(semaphore->handle) == 0)
            status = true;
    }

    return status;
}

bool POSIX_Semaphore_Wait(POSIX_Semaphore *semaphore)
{
    bool status = false;
    if(semaphore && semaphore->handle)
    {
        if(sem_wait(semaphore->handle) == 0)
            status = true;
    }
    return status;
}

bool POSIX_Semaphore_Cleanup(POSIX_Semaphore *semaphore)
{
    bool status = false;
    if(semaphore && semaphore->handle)
    {
        sem_close(semaphore->handle);
        sem_unlink(semaphore->name);
        status = true;
    }

    return status;
}
