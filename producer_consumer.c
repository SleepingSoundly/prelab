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
  // TODO need a conditional variable here
} queue_t;


/* Thread Function Prototypes */
void *producer_routine(void *arg);
void *consumer_routine(void *arg);


/* Global Data */
long g_num_prod; /* number of producer threads */
pthread_mutex_t g_num_prod_lock;

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

  // TODO-> the below was not initialized
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

  // TODO thread return was a *(long *)thread_return, when it's printed it should just be cast
  printf("\nFirst Thread Printed %lu characters.\n", (long)thread_return);

  //free(thread_return);


  pthread_cond_destroy(&more);
  pthread_mutex_destroy(&queue.lock);
  pthread_mutex_destroy(&g_num_prod_lock);

  //TODO -> IS pthread_exit(NULL); Necessary? at the end? 
  printf("DEBUG: %lu main exit\n", pthread_self());

  // This is a pthread_exit so it allows the consumer to terminate on it's own, and doesn't destroy resources that
  // the detached consumer needs
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

  //result = pthread_detach(consumer_thread);
  //if (0 != result)
  //  fprintf(stderr, "Failed to detach consumer thread: %s\n", strerror(result));


  for (c = 'a'; c <= 'z'; ++c) {

    /* Create a new node with the prev letter */
    new_node_p = malloc(sizeof(queue_node_t));
    new_node_p->c = c;
    new_node_p->next = NULL;

    /* Add the node to the queue */

    pthread_mutex_lock(&queue_p->lock);
    //while(queue_p->front != NULL){
    //  printf("Producer waiting for queue\n");
    //  pthread_cond_wait(&more, &queue_p->lock);
    //}

    //printf("PRODUCING\n");
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

    // TODO let everybody know there's something in the queue
    //printf("MORE\n");
    pthread_cond_signal(&more);
    pthread_mutex_unlock(&queue_p->lock);
    sched_yield();
  }

  /* Decrement the number of producer threads running, then return */
  pthread_mutex_lock(&g_num_prod_lock);
  --g_num_prod;
  pthread_mutex_unlock(&g_num_prod_lock);

  //printf("DEBUG: DONE WITH PRODUCING\n");

  // TODO-> Consumer thread probably finished before this one would finish, would try to take out a destroyed lock

  //result = pthread_join(consumer_thread, &thread_return);
  //if (0 != result) {
  //  fprintf(stderr, "Failed to join consumer thread: %s\n", strerror(result));
  //  pthread_exit(NULL);
  //}
  //else
  // printf("Producer's consumer is dead\n");
  // keeping the create/cancel, because it maintains controllability from the parent process
  result = pthread_cancel(consumer_thread);
  if (0 != result){
    fprintf(stderr, "Failed to join consumer thread: %s\n", strerror(result));
    pthread_exit(NULL);
  }

  printf("DEBUG: %lu EXIT\n", pthread_self());
  // TODO all these threads should pthread_exit with end arguments to exit, not just return with an arg. 
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

  // I believe there's no reason for the consumer to take out the prod lock thread

  pthread_mutex_lock(&queue_p->lock);
  // wait until there's something in the queue and then
  // wake up, it will get the queue lock next
  pthread_mutex_lock(&g_num_prod_lock);

  // the gating predicate for starting to consume is a little more complicated, because we need to
  // know if there is more, but also to check to make sure the other thread didn't already 
  // eat everything
  while(queue_p->front == NULL && g_num_prod > 0){
    pthread_mutex_unlock(&g_num_prod_lock);
    //printf("DEBUG: %lu waiting for more signal to take the lock\n", pthread_self());
    pthread_cond_wait(&more, &queue_p->lock);
    pthread_mutex_lock(&g_num_prod_lock);
  }


  // there's a producer lock because we need to know when it's done 
  //pthread_mutex_lock(&g_num_prod_lock);
  //printf("DEBUG: have the number lock\n");

  // "rechecking the predicate" should be contingent on a wait
  while(queue_p->front != NULL || g_num_prod > 0) {
    

    pthread_mutex_unlock(&g_num_prod_lock);
    // unlocking this to make sure we don't block the end of the producer thread
 
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

      //printf("DEBUG: %lu yields\n", pthread_self());
      sched_yield();

      // once it's done yielding, this thread should pick the locks back up
      // either way, need the locks back for the queue and the number of products

      

      //while(queue_p->front == NULL){
      //  printf("DEBUG: %lu waiting for MORE\n", pthread_self());
      //  pthread_cond_wait(&more, &queue_p->lock);
      //}

    }

    pthread_mutex_lock(&queue_p->lock);
    pthread_mutex_lock(&g_num_prod_lock);

  }


  pthread_mutex_unlock(&g_num_prod_lock);

  pthread_cond_signal(&more);
  pthread_mutex_unlock(&queue_p->lock);
  // signal any thread waiting for more to wake up and find
  // that there isn't anything left to consume


  printf("DEBUG: %lu EXIT\n", pthread_self());
  pthread_exit((void*) count);
}
