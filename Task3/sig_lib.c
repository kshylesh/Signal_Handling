
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#define MOUSE_FILE "/dev/input/event4"

/*A node representation of the linked list
that needs to be created containing the thread id 
of the thread to which the signal needs to be sent*/
struct sig_list
{
    pthread_t sig_thread;
    struct sig_list *next;
};
/*Pointers to head and current nodes of a list at any instance of time*/
struct sig_list *head = NULL, *current = NULL;
/*A flag to stop the background event detection thread in the 
library*/
volatile bool clean_up = false;

pthread_t background_thread_t_id;

/*This function takes the thread id as the parameter which
wants the event update of signal SIGIO,and puts it in a linked list*/
int sig_handler_reg(pthread_t t_id)
{
    printf("Inside %s\n",__FUNCTION__);
	struct sig_list *traverse_node;
	/*Creates a node from heap*/
	traverse_node = (struct sig_list*)malloc(sizeof(struct sig_list));
		if (traverse_node==NULL)
		{
			printf("Node Creation failed \n");
			return -1;
		}
		/*Upon successfull memory allocation,fills it with
		passed thread ID and the next link as NUll*/
	else
	{
	    traverse_node->sig_thread = t_id;
		traverse_node->next = NULL;	
		}
		/*If the list is empty , make the node as the
		head of the list*/
	if (head==NULL)
	{	
			head = traverse_node;
			current = traverse_node;
		
	}
	/*Append the node to the list if the linked
	list is not empty*/
	else
	{
			current->next = traverse_node;
			current = traverse_node;
		
	}

	return 0;
}

/*Searches for a node with a given thread id from linked list,
Fills the reference of a previous node to the node which needs to
be deleted in the "prev_node",so that link can be made fromprev node
to next node of the node to be deleted*/
static struct sig_list* give_node(pthread_t t_id, struct sig_list **prev_node)
{
    struct sig_list *traverse_node = head;
    struct sig_list *temp_node = NULL;
    bool thread_id_in_list = false;
    /*Loop through the linked list*/
    while(traverse_node != NULL)
    {
        /*Break from the list if the node with specified 
        thread id is found*/
        if(traverse_node->sig_thread == t_id)
        {
            thread_id_in_list = true;
            break;
        }
        else
        {
            /*Else traverse to next node in list*/
            temp_node = traverse_node;
            traverse_node = traverse_node->next;
        }
    }
/*If node is found,then fill the pointer with node's
previous node reference andreturn the node*/
    if(thread_id_in_list==true)
    {
        if(prev_node)
            *prev_node = temp_node;
        return traverse_node;
    }
    /*Return NUll if no such node is found*/
    else
    {
        return NULL;
    }
}

/*This Library function dequeues the node from the list 
which has a specific thread id as specified in the parameter*/
int sig_handler_unreg(pthread_t t_id)
{
    struct sig_list *prev_node = NULL;
    struct sig_list *node_to_delete = NULL;
    
    /*Gets the reference of the node to be deleted*/
    node_to_delete = give_node(t_id,&prev_node);
    if(node_to_delete == NULL)
    {
        return -1;
    }
    else
    {
        /*Links the prev node to next node of present node
        taht needs to be deleted*/
        if(prev_node != NULL)
            prev_node->next = node_to_delete->next;

        if(node_to_delete == current)
        {
            current = prev_node;
        }
        /*If the node to be deleted is head of the list then 
        make the next node as the linked list head*/
        else if(node_to_delete == head)
        {
            head = node_to_delete->next;
        }
    }
    /*free the node's memory in heap*/
    free(node_to_delete);
    node_to_delete = NULL;

    return 0;
}

/*A daemon signal handler that sends a signal to every registered
thread in a linked list for SIGIO signal,when a event corrsponding
to the SIGIO generation takes place*/

void send_kill_to_all_reg_threads(int sig_dummy)
{
	struct sig_list *traverse_node = head;
    printf("------------------------------------\n");
    printf("Inside %s\n",__FUNCTION__);
    printf("------------------------------------\n");
	/*Loop through the list*/
    while (traverse_node != NULL)
    {
    /*Send every thread the signal */
		pthread_kill(traverse_node->sig_thread,SIGUSR1);
        traverse_node = traverse_node->next;
    }
    return;
}

/*Background thread that detects asynchronous event from 
mouse which receives the SIGIO, hence redirecting the event
occurances to every possible thread by the signal handler*/ 
void* background_thread(void* dummy)
{
	struct sigaction sig_action;
	sigset_t block_mask;
	int mouse_file_desc,ret,flags;
	
	
	if((mouse_file_desc = open(MOUSE_FILE, O_RDONLY)) < 0)
	{
		perror("Error Opening Mouse file:");
		return 0;
	}

	memset(&sig_action, 0, sizeof(sig_action));
	sigfillset(&block_mask);
	sig_action.sa_mask = block_mask;
	/*Registers the signal handler for SIGIO*/
	sig_action.sa_handler = &send_kill_to_all_reg_threads;
	sigaction(SIGIO, &sig_action, NULL);
	
	ret = fcntl(mouse_file_desc, F_SETOWN, getpid());
	if (ret < 0)
	{
		perror("fcntl1");
	}
	flags = fcntl(mouse_file_desc, F_GETFL);
	/*Sets the device file to recognize async events*/
	fcntl(mouse_file_desc, F_SETFL, flags | FASYNC);
	
	/*Loop infinetely until clean_up flag is
	set by the clean up library interface called
	by the application program after run time
	expiration*/
	while(clean_up == false);
	return NULL;
}

/*Library initialization function,which sets up a background
thread to detect mouse events and send the notification of
the received SIGIO for each registered thread*/
void sig_lib_init(void)
{
	printf("Inside %s\n",__FUNCTION__);
	pthread_create(&background_thread_t_id,NULL,&background_thread,NULL);
    return;
}

/*Library call to terminate the background event detection thread*/
void sig_lib_clean_up(void)
{
	clean_up = true;
	pthread_join(background_thread_t_id,NULL);
    return;
}
