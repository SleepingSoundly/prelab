/*
 *  Pre-lab Summary:
 *      There are five bugs here, and a couple of other small things I fixed on the side. 
 *
 *        1. There needed to be a conditional variable on the queue. The producer/consumer model isn't very affective without one
 *            because the consumers need to know when to consume, not just when the lock is free. A conditional called "More" was
 *            added below
 *        2. The producer thread was detached but the code expected it to join back. I chose to remove the pthread_detatch function
 *        3. The use of g_num_prod needed to be locked in the producor thread (the -- operation), because that's used as a predicate
 *            to the use of the consumer thread
 *        4. The Consumer needed to use pthread_exit, not return. main was expecting the thread to rejoin with a return variable. 
 *        5. There was a bad pointer de-reference on the thread_return variable that was causing aborts and segmentation faults
 *           The print statement was fixed and the "free" was removed because it wasn't necessary. 
 *
 *  Note-> There was some extra code added to make sure all these bugs (particularly the conceptual bugs) were fixed. 
 *          Bug fixed are denoted by "// " comments, as to make them stick out against the original code. 
 */

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>


/* Queue Structures */

typedef struct queue_node_s {
  struct queue_node_s *next;
  struct queue_node_s *prev;
  char c;
} queue_node_t;

typedef struct {
  struct queue_node_s *front;
  struct queue_node_s *back;
  pthread_mutex_t lock;
} queue_t;


/* Thread Function Prototypes */
void *producer_routine(void *arg);
void *consumer_routine(void *arg);


/* Global Data */
long g_num_prod; /* number of producer threads */
pthread_mutex_t g_num_prod_lock;


// BUG: "needs a conditional"
// this whole model needs a conditional variable to tell the consumers when it's ok to
// begin consuming. It's marked all over the code where "more" is used, but documented here

// conditional variable to signal that the queue is available 
pthread_cond_t more = PTHREAD_COND_INITIALIZER;

/* Main - entry point */
int main(int argc, char **argv) {
  queue_t queue;
  pthread_t producer_thread, consumer_thread;
  void *thread_return = NULL;
  int result = 0;

  /* Initialization */

  printf("Main thread started with thread id %lu\n", pthread_self());
  // TODO maybe the sizeof should be queue_t ?? 
  memset(&queue, 0, sizeof(queue));

  pthread_mutex_init(&queue.lock, NULL);

  // BUG: the below was not initialized
  pthread_mutex_init(&g_num_prod_lock, NULL);


  pthread_mutex_lock(&g_num_prod_lock);
  g_num_prod = 1; /* there will be 1 producer thread */
  pthread_mutex_unlock(&g_num_prod_lock);

  /* Create producer and consumer threads */

  result = pthread_create(&producer_thread, NULL, producer_routine, &queue);
  if (0 != result) {
    fprintf(stderr, "Failed to create producer thread: %s\n", strerror(result));
    exit(1);
  }

  printf("Producer thread started with thread id %lu\n", producer_thread);

  // TODO -> CANNOT DETACH FROM SOMETHING YOU EXPECT TO JOIN
  // ORIGINAL LINE: 
  // result = pthread_detach(producer_thread);
  // ADJUSTED LINE: 
  // Nothing, it should not detatch
  //if (0 != result)
  //  fprintf(stderr, "Failed to detach producer thread: %s\n", strerror(result));

  result = pthread_create(&consumer_thread, NULL, consumer_routine, &queue);
  if (0 != result) {
    fprintf(stderr, "Failed to create consumer thread: %s\n", strerror(result));
    // TODO this should be a pthread exit, since there is another thread already stared
    pthread_exit(NULL);
  }

  /* Join threads, handle return values where appropriate */

  result = pthread_join(producer_thread, &thread_return);
  if (0 != result) {
    fprintf(stderr, "Failed to join producer thread: %s\n", strerror(result));
    pthread_exit(NULL);
  }
  else
    printf("Producer is dead\n");



  result = pthread_join(consumer_thread, &thread_return);
  if (0 != result) {
    fprintf(stderr, "Failed to join consumer thread: %s\n", strerror(result));
    pthread_exit(NULL);
  }
  else
    printf("Consumer is dead\n");

  // BUG: 
  // Thread return was a *(long *)thread_return, when it's printed it should just be cast
  // this would cause an aborted process after the threads finished and we returned here
  printf("\nFirst Thread Printed %lu characters.\n", (long)thread_return);

  // BUG:
  // additionally, the mishandling of thread_return extended to free-ing this variable
  // it would also cause an aborted main
  // -----> free(thread_return);


  pthread_cond_destroy(&more);
  pthread_mutex_destroy(&queue.lock);
  pthread_mutex_destroy(&g_num_prod_lock);

  printf("DEBUG: %lu main exit\n", pthread_self());

  // This wasn't necessariliy a bug, but it's better practice to call pthread_exit in these cases
  // to make sure that we don't destroy resources that are still in use. it will call exit() on it's own
  pthread_exit(NULL);
}


/* Function Definitions */

/* producer_routine - thread that adds the letters 'a'-'z' to the queue */
void *producer_routine(void *arg) {
  queue_t *queue_p = arg;
  queue_node_t *new_node_p = NULL;
  pthread_t consumer_thread;
  int result = 0;
  char c;
  void *thread_return = NULL;

  result = pthread_create(&consumer_thread, NULL, consumer_routine, queue_p);
  if (0 != result) {
   fprintf(stderr, "Failed to create consumer thread: %s\n", strerror(result));
    exit(1);
  }
  else
    printf("DEBUG: created consumer from producer\n");


  for (c = 'a'; c <= 'z'; ++c) {

    /* Create a new node with the prev letter */
    new_node_p = malloc(sizeof(queue_node_t));
    new_node_p->c = c;
    new_node_p->next = NULL;

    /* Add the node to the queue */

    pthread_mutex_lock(&queue_p->lock);

    if (queue_p->back == NULL) {
      assert(queue_p->front == NULL);
      new_node_p->prev = NULL;

      queue_p->front = new_node_p;
      queue_p->back = new_node_p;
    }
    else {
      assert(queue_p->front != NULL);
      new_node_p->prev = queue_p->back;
      queue_p->back->next = new_node_p;
      queue_p->back = new_node_p;
    }

    // Part of the overall "needs a conditional" bug
    pthread_cond_signal(&more);
    pthread_mutex_unlock(&queue_p->lock);
    sched_yield();
  }

  /* Decrement the number of producer threads running, then return */
  pthread_mutex_lock(&g_num_prod_lock);
  --g_num_prod;
  pthread_mutex_unlock(&g_num_prod_lock);

  // BUG:
  // the thread created by this process needs to be cancelled, otherwise you end up in a lost wait situation
  // this is part of the "needs a conditional" bug, as it is contingent on a wait caused by the signal
  // used for the conditional
  result = pthread_cancel(consumer_thread);
  if (0 != result){
    fprintf(stderr, "Failed to join consumer thread: %s\n", strerror(result));
    pthread_exit(NULL);
  }

  printf("DEBUG: %lu EXIT\n", pthread_self());
  pthread_exit((void*) 0);

}


/* consumer_routine - thread that prints characters off the queue */
void *consumer_routine(void *arg) {
  queue_t *queue_p = arg;
  queue_node_t *prev_node_p = NULL;
  long count = 0; /* number of nodes this thread printed */

  printf("Consumer thread started with thread id %lu\n", pthread_self());

  /* terminate the loop only when there are no more items in the queue
   * AND the producer threads are all done */

  pthread_mutex_lock(&queue_p->lock);
  pthread_mutex_lock(&g_num_prod_lock);

  // BUG: "needs a conditional"
  // the gating predicate for starting to consume is a little more complicated, because we need to
  // know if there is more, but also to check to make sure the other thread didn't already 
  // eat everything
  while(queue_p->front == NULL && g_num_prod > 0){
    pthread_mutex_unlock(&g_num_prod_lock);
    pthread_cond_wait(&more, &queue_p->lock);
    pthread_mutex_lock(&g_num_prod_lock);
  }


  // "rechecking the predicate" should be contingent on a wait
  while(queue_p->front != NULL || g_num_prod > 0) {
    

    pthread_mutex_unlock(&g_num_prod_lock);

 
    if (queue_p->front != NULL) {      
      
      /* Remove the prev item from the queue */
      prev_node_p = queue_p->front;

      if (queue_p->front->next == NULL)
        queue_p->back = NULL;
      else
        queue_p->front->next->prev = NULL;

      queue_p->front = queue_p->front->next;
      pthread_cond_signal(&more);
      pthread_mutex_unlock(&queue_p->lock);

      /* Print the character, and increment the character count */
      printf("%lu: %c\n", pthread_self(), prev_node_p->c);
      free(prev_node_p);
      ++count;

    }
    else { /* Queue is empty, so let some other thread run */

      pthread_cond_signal(&more);
      pthread_mutex_unlock(&queue_p->lock);


      sched_yield();

      // once it's done yielding, this thread should pick the locks back up
      // either way, need the locks back for the queue and the number of products
    }
    // locks need to be picked back up at the end here regardless of wait
    // if it already has this lock, the queue lock is a NOP
    pthread_mutex_lock(&queue_p->lock);
    pthread_mutex_lock(&g_num_prod_lock);

  }


  pthread_mutex_unlock(&g_num_prod_lock);

  pthread_cond_signal(&more);
  pthread_mutex_unlock(&queue_p->lock);

  printf("DEBUG: %lu EXIT\n", pthread_self());
  pthread_exit((void*) count);
}
