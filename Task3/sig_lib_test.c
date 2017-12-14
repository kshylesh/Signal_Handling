#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include "sig_lib.h"
#define APP_TIME 120

/*Thread id of 6 threads to which the signal needs to
be sent*/
pthread_t t_id[6];
int ter=0;
/*Mutex to synchronize data between signal handler and the
main thread*/
pthread_mutex_t p_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t flag_lock = PTHREAD_MUTEX_INITIALIZER;

/*Signal handler,which is invoked when the signal is sent to threads,
depending on which thread received the signal i,e by pthread id it displays
the message for that thread*/
void sig_display_msg(int sig_dummy)
{
	pthread_mutex_lock(&p_mutex);
	if (pthread_equal(pthread_self(),t_id[0]))
	{
		printf(" Thread0 signal handler was called by the SIGIO signal\n");
	}
	else if (pthread_equal(pthread_self(),t_id[1]))
	{
		printf("Thread1 signal handler was called by the SIGIO signal\n");
	}
	else if (pthread_equal(pthread_self(),t_id[2]))
	{
		printf(" Thread2 signal handler was called by the SIGIO signal\n");
	}
	else if (pthread_equal(pthread_self(),t_id[3]))
	{
		printf(" Thread3 signal handler was called by the SIGIO signal\n");
	}
	else if (pthread_equal(pthread_self(),t_id[4]))
	{
		printf(" Thread4 signal handler was called by the SIGIO signal\n");
	}
	else if (pthread_equal(pthread_self(),t_id[5]))
	{
		printf(" Thread5 signal handler was called by the SIGIO signal\n");
	}
	else
	{

		printf(" Error signal handling\n");

	}
	pthread_mutex_unlock(&p_mutex);
    return;
}

/*A signal handler registration threads which registers the signal 
handler for each thread and busy loops until a termination flag is set
By main thread upon expiration of application run time*/
void* sig_reg_thread(void* dummy)
{
	struct sigaction sig_action;
	sigset_t block_mask;
	
   
	memset(&sig_action, 0, sizeof(sig_action));
	sigfillset(&block_mask);
	sig_action.sa_mask = block_mask;
	sig_action.sa_handler = &sig_display_msg;
	sigaction(SIGUSR1, &sig_action, NULL);
	while(1)
	{   
	    /*As termination flag is shared between this context and the
	    main thread , it needs to locked before accessing to eradicate
	    data inconsistancy*/
        pthread_mutex_lock(&flag_lock);
        if(ter==1)
        {
            pthread_mutex_unlock(&flag_lock);
            break;
         }
        else    
        pthread_mutex_unlock(&flag_lock);
     } 
    return NULL;       
}

int main()
{
	
	int i;
	/*Set the application run time which would be
	used by nanosleep*/
    struct timespec sleep_time = {APP_TIME,0},rem;
    
    
	pthread_mutex_lock(&p_mutex);
	
	/*All the threads are created ,and their respective
	thread functionality will register the handler for them*/
	pthread_create(&t_id[0],NULL,&sig_reg_thread,NULL);
	pthread_create(&t_id[1],NULL,&sig_reg_thread,NULL);
	pthread_create(&t_id[2],NULL,&sig_reg_thread,NULL);
	pthread_create(&t_id[3],NULL,&sig_reg_thread,NULL);
	pthread_create(&t_id[4],NULL,&sig_reg_thread,NULL);
	pthread_create(&t_id[5],NULL,&sig_reg_thread,NULL);

	/*This call initializes the library which in turn will 
	create a background thread which detects the mouse I/O events
	in an asynchronous manner*/
	sig_lib_init();
	
	for(i = 0; i < 6; i++)
	{
	    /*This library call is made so that thread id are put into a 
	    linked list ,so that when SIGIO is received by background thread
	    , a signal can be sent to all the threads in the linked list*/
	    
		if (sig_handler_reg(t_id[i]) < 0)
		{
			printf("Error registering the SIGIO for Thread%d \n",i);
		}
	}
	
	pthread_mutex_unlock(&p_mutex);
	
	
	printf("Create mouse signal by scrolling mouse up or down or clicking the button,app runs for %d sec\n",APP_TIME);
	
	/*Main thread sleeps for application run time and if it is interrupted by 
	signal then puts the remaining sleep time in "rem",which is used to
	complete the sleep interval later*/
	while(nanosleep(&sleep_time,&rem)!=0)
	{
		memcpy(&sleep_time,&rem,sizeof(struct timespec));
	}
	
	pthread_mutex_lock(&p_mutex);
	for(i = 0; i < 6; i++)
	{
	    /*After the tasks are completed and application run time is expired,
	    delete the thread id from linked list and clean up resources*/
		if (sig_handler_unreg(t_id[i]) < 0)
		{
			printf("Error registering the SIGIO for Thread%d \n",i);
		}
	}
	/*Set the termination flag so that the created child threads 
	return to main thread*/
	ter=1;
	pthread_mutex_unlock(&flag_lock);
	/*Wait till all the child threads are joined*/
	for(i = 0; i < 6; i++)
	{
		pthread_join(t_id[i],NULL);
	}
	/*Clean up library resource i,e stop the background 
	mouse detection thread in the library*/
	sig_lib_clean_up();
	return 0;
}
