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

#define QUEUESIZE 10
#define LOOP 1000

int work_num = 0;

void *producer (void *args);
void *consumer (void *args);

typedef struct workFunction {
  void * (*work)(void *);
  void * arg;
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

  if (argc != 3) {
    fprintf(stderr, "Usage: <P> <Q>\n");
    exit(1);
  }

  int P = atoi(argv[1]);
  int Q = atoi(argv[2]);

  pro = malloc(sizeof(pthread_t) * P);
  if (!pro) { perror("Failed to create producers"); exit(1); }

  con = malloc(sizeof(pthread_t) * Q);
  if (!con) { perror("Failed to create consumers"); exit(1); }

  fifo = queueInit ();
  if (fifo ==  NULL) {
    fprintf (stderr, "main: Queue Init failed.\n");
    exit (1);
  }

  for (int i = 0; i < P; i++) {
    pthread_create (&pro[i], NULL, producer, fifo);
  }

  for (int i = 0; i < Q; i++) {
    pthread_create (&con[i], NULL, consumer, fifo);
  }

  for (int i = 0; i < P; i++) {
    pthread_join (pro[i], NULL);
  }

  for (int i = 0; i < Q; i++) {
    pthread_join (con[i], NULL);
  }

  free(pro);
  free(con);

  queueDelete (fifo);

  return 0;
}

void *dummy_work (void *arg) {
    usleep(1000);
    return NULL;
}

void *producer (void *q)
{
  queue *fifo;
  int i;

  fifo = (queue *)q;

  for (i = 0; i < LOOP; i++) {
    pthread_mutex_lock (fifo->mut);
    while (fifo->full) {
      printf ("producer: queue FULL.\n");
      pthread_cond_wait (fifo->notFull, fifo->mut);
    }
    int *work_n = (int *)malloc(sizeof(int)); 
    if (!work_n) { perror("producer failed to create work"); exit(1); }
    *work_n = ++work_num;

    workFunction wf = {dummy_work, (void *)work_n};
    queueAdd (fifo, wf);
    printf("producer created work %d\n", *work_n);
    pthread_mutex_unlock (fifo->mut);
    pthread_cond_signal (fifo->notEmpty);
  }

  return (NULL);
}

void *consumer (void *q)
{
  queue *fifo;

  fifo = (queue *)q;
  workFunction wf;

  while (1) {
    pthread_mutex_lock (fifo->mut);
    while (fifo->empty) {
      printf ("consumer: queue EMPTY.\n");
      pthread_cond_wait (fifo->notEmpty, fifo->mut);
    }
    queueDel (fifo, &wf);
    pthread_mutex_unlock (fifo->mut);
    pthread_cond_signal (fifo->notFull);
    wf.work(wf.arg);
    printf ("consumer: finished work %d.\n", *((int *)wf.arg));
    if (*(wf.arg) == LOOP * P) break;
    free(wf.arg);
  }

  return (NULL);
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
