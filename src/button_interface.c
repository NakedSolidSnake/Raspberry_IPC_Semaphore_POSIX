#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <unistd.h>
#include <button_interface.h>

#define _1ms    1000

static void wait_press(void *object, Button_Interface *button)
{
    while (true)
    {
        if (!button->Read(object))
        {
            usleep(_1ms * 100);
            break;
        }
        else
        {
            usleep(_1ms);
        }
    }
}

bool Button_Run(void *object, POSIX_Semaphore *semaphore, Button_Interface *button)
{
    if(button->Init(object) == false)
		return false;

    if(POSIX_Semaphore_Create(semaphore) == false)
        return false;

    while(true)
	{
        wait_press(object, button);
        POSIX_Semaphore_Post(semaphore);
	}

    POSIX_Semaphore_Cleanup(semaphore);
   
    return false;
}
