#include <stdio.h> 
#include <stdlib.h>
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <unistd.h>
#include <led_interface.h>

#define _1ms    1000

bool LED_Run(void *object, POSIX_Semaphore *semaphore, LED_Interface *led)
{
	int status = 0;

	if(led->Init(object) == false)
		return false;

	if(POSIX_Semaphore_Create(semaphore) == false)
		return false;		

	while(true)
	{
		if(POSIX_Semaphore_Wait(semaphore) == true)
		{
			status ^= 0x01; 
			led->Set(object, status);
		}
	}

	return false;	
}
