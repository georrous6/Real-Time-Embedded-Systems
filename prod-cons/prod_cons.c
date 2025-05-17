/*
 *	File	: pc.c
 *
 *	Title	: Demo Producer/Consumer.
 *
 *	Short	: A solution to the producer consumer problem using
 *		pthreads.	
 *
 *	Long 	:
 *
 *	Author	: Andrae Muys
 *
 *	Date	: 18 September 1997
 *
 *	Revised	:
 */

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>

#define QUEUESIZE 10
#define LOOP 10000

int producers_done = 0;
int P = 0, Q = 0;
long total_wait_us = 0;
pthread_mutex_t wait_time_mutex = PTHREAD_MUTEX_INITIALIZER;

void *producer (void *args);
void *consumer (void *args);

typedef struct workFunction {
  void * (*work)(void *);
  void * arg;
  struct timeval timestamp;  // time when producer adds work
} workFunction;

void *dummy_work(void *arg);

typedef struct {
  workFunction buf[QUEUESIZE];
  long head, tail;
  int full, empty;
  pthread_mutex_t *mut;
  pthread_cond_t *notFull, *notEmpty;
} queue;

queue *queueInit (void);
void queueDelete (queue *q);
void queueAdd (queue *q, workFunction in);
void queueDel (queue *q, workFunction *out);

int main (int argc, char* argv[])
{
  queue *fifo;
  pthread_t *pro, *con;

  if (argc != 4) {
    fprintf(stderr, "Usage: %s <P> <Q> <output>\n", argv[0]);
    exit(1);
  }

  P = atoi(argv[1]);
  Q = atoi(argv[2]);
  const char* filename = argv[3];

  pro = malloc(sizeof(pthread_t) * P);
  if (!pro) { perror("Failed to create producers"); exit(1); }

  con = malloc(sizeof(pthread_t) * Q);
  if (!con) { perror("Failed to create consumers"); exit(1); }

  fifo = queueInit ();
  if (fifo ==  NULL) {
    fprintf (stderr, "main: Queue Init failed.\n");
    exit (1);
  }

  for (int i = 0; i < P; i++)
    pthread_create (&pro[i], NULL, producer, fifo);

  for (int i = 0; i < Q; i++)
    pthread_create (&con[i], NULL, consumer, fifo);

  for (int i = 0; i < P; i++)
    pthread_join (pro[i], NULL);

  for (int i = 0; i < Q; i++)
    pthread_join (con[i], NULL);

  free (pro);
  free (con);
  queueDelete (fifo);

  double average_waiting_time = (double)total_wait_us / (P * LOOP);
  printf("Average wait time: %.2f us\n", average_waiting_time);
  FILE* fp = fopen(filename, "a");
  if (!fp) {
    fprintf(stderr, "Failed to open \'%s\': %s\n", filename, strerror(errno));
    exit(1);
  }

  fprintf(fp, "%d %d %lf\n", P, Q, (double)total_wait_us / (P * LOOP));
  fclose(fp);

  return 0;
}

void *dummy_work (void *arg) {
    // usleep(1000);
    return NULL;
}

void *producer (void *q)
{
  queue *fifo = (queue *)q;

  for (int i = 0; i < LOOP; i++) {
    pthread_mutex_lock (fifo->mut);
    while (fifo->full)
      pthread_cond_wait (fifo->notFull, fifo->mut);

    workFunction wf;
    wf.work = dummy_work;
    wf.arg = NULL;
    gettimeofday(&wf.timestamp, NULL);
    queueAdd (fifo, wf);
    pthread_mutex_unlock (fifo->mut);
    pthread_cond_signal (fifo->notEmpty);
  }

  pthread_mutex_lock(fifo->mut);
  producers_done++;
  pthread_mutex_unlock(fifo->mut);
  if (producers_done == P)
      pthread_cond_broadcast(fifo->notEmpty);  // wake up all consumers

  return (NULL);
}

void *consumer (void *q)
{
  queue *fifo = (queue *)q;
  workFunction wf;
  struct timeval now;

  while (1) {
    pthread_mutex_lock (fifo->mut);
    while (fifo->empty) {
      if (producers_done == P) {
        pthread_mutex_unlock (fifo->mut);
        return NULL;  // All producers done, exit
      }
      pthread_cond_wait (fifo->notEmpty, fifo->mut);
    }
    queueDel (fifo, &wf);
    pthread_mutex_unlock (fifo->mut);
    pthread_cond_signal (fifo->notFull);

    gettimeofday(&now, NULL);

    long wait_us = (now.tv_sec - wf.timestamp.tv_sec) * 1000000L +
                    (now.tv_usec - wf.timestamp.tv_usec);

    pthread_mutex_lock(&wait_time_mutex);
    total_wait_us += wait_us;
    pthread_mutex_unlock(&wait_time_mutex);

    wf.work(wf.arg);
  }
}

queue *queueInit (void)
{
  queue *q;

  q = (queue *)malloc (sizeof (queue));
  if (q == NULL) return (NULL);

  q->empty = 1;
  q->full = 0;
  q->head = 0;
  q->tail = 0;
  q->mut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
  pthread_mutex_init (q->mut, NULL);
  q->notFull = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (q->notFull, NULL);
  q->notEmpty = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (q->notEmpty, NULL);
	
  return (q);
}

void queueDelete (queue *q)
{
  pthread_mutex_destroy (q->mut);
  free (q->mut);	
  pthread_cond_destroy (q->notFull);
  free (q->notFull);
  pthread_cond_destroy (q->notEmpty);
  free (q->notEmpty);
  free (q);
}

void queueAdd (queue *q, workFunction in)
{
  q->buf[q->tail] = in;
  q->tail++;

  if (q->tail == QUEUESIZE)
    q->tail = 0;

  if (q->tail == q->head)
    q->full = 1;

  q->empty = 0;

  return;
}

void queueDel (queue *q, workFunction *out)
{
  *out = q->buf[q->head];

  q->head++;

  if (q->head == QUEUESIZE)
    q->head = 0;

  if (q->head == q->tail)
    q->empty = 1;

  q->full = 0;

  return;
}
