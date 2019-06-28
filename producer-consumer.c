#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_LEN 100  // max length of buffer
#define SUCCESS 0

// global variables
// a buffer/ queue to store goods
struct buffer {
  // use positive int to represent goods, 0 <=> empty
  // MAX_LEN + 1: to make a queue/ pool, +1 is needed
  int items[MAX_LEN];
  // the head and tail of the queue of goods
  int head, tail;
  // mutex locks for queue
  pthread_mutex_t mutex;
} goodsBuf;

// semaphores indicating the stock
sem_t full, empty;

// functions to handle buffer
int getItem() {  // dequeue
  // semaphore guarantees buffer isn't empty
  int ans = goodsBuf.items[goodsBuf.head];
  goodsBuf.items[goodsBuf.head] = 0;  // clear
  goodsBuf.head = (goodsBuf.head + 1) % MAX_LEN;
  return ans;
}
void putItem(int item) {
  // semaphore guarantees buffer isn't full
  goodsBuf.items[goodsBuf.tail] = item;
  // tail++ in round queue
  goodsBuf.tail = (goodsBuf.tail + 1) % MAX_LEN;
}

// a function to read goods from file,
// returns the amount of read goods
int readBuffer(char *fileLoc) {
  FILE *source = fopen(fileLoc, "r");
  if (source == NULL) {
    printf("File doesn't exist\n");
    return 0;
  }

  int tmp, count = 0;
  // read until EOF, incrementing counter and queue head
  for (; fscanf(source, "%d", &tmp) != EOF; count++) {
    putItem(tmp);
  }
  return count;
}

void *consume() {
  // roughly get 2-digit thread id
  int id = pthread_self() % 100;
  srand(id);  // init the random seed
  printf("Consumer %d stated\n", id);
  while (1) {
    if (sem_wait(&full) != SUCCESS) {
      perror("waiting for semaphore");
    }
    if (pthread_mutex_lock(&goodsBuf.mutex) != SUCCESS) {
      perror("locking mutex");
    }

    usleep(rand() % 1000000);  // consuming for 0-1s
    printf("[C%.2d] consumed good %d\n", id, getItem());

    if (pthread_mutex_unlock(&goodsBuf.mutex) != SUCCESS) {
      perror("unlocking mutex");
    }
    if (sem_post(&empty) != SUCCESS) {
      perror("signaling");
    }
  }
}

void *produce() {
  // roughly get 2-digit thread id
  int id = pthread_self() % 100;
  srand(id);  // init the random seed
  printf("Producer %d stated\n", id);
  while (1) {
    if (sem_wait(&empty) != SUCCESS) {
      perror("waiting for semaphore");
    }
    if (pthread_mutex_lock(&goodsBuf.mutex) != SUCCESS) {
      perror("locking mutex");
    }

    usleep(rand() % 1000000);  // producing for 0-1s
    int item = rand() % 100;   // 2-digit int representing good
    printf("[P%.2d] produced good %d\n", id, item);
    putItem(item);

    if (pthread_mutex_unlock(&goodsBuf.mutex) != SUCCESS) {
      perror("unlocking mutex");
    }
    if (sem_post(&full) != SUCCESS) {
      perror("signaling");
    }
  }
}

int main() {
  // init the buffer
  char *fileLoc = "pre-buffer.txt";
  goodsBuf.head = goodsBuf.tail = 0;
  int amt = readBuffer(fileLoc);
  if (amt == 0) {
    printf("failed to read existing buffer\n");
    return 1;
  }
  // init the semaphores
  if (sem_init(&full, 0, amt)
      // -1: an unusable slot in queue
      && sem_init(&empty, 0, MAX_LEN - amt - 1) != SUCCESS) {
    perror("initializing semaphore");
    return 1;
  }
  // init mutex
  pthread_mutexattr_t mutexAttr;
  if (pthread_mutexattr_init(&mutexAttr) &&
      pthread_mutex_init(&goodsBuf.mutex, &mutexAttr) != SUCCESS) {
    perror("initializing mutex");
    return 1;
  }

  pthread_t tid;
  // create 3 consumer threads
  for (int i = 0; i < 3; i++) {
    if (pthread_create(&tid, NULL, consume, NULL) != SUCCESS) {
      perror("creating threads");
      return 1;
    }
  }
  // create 4 producer threads
  for (int i = 0; i < 4; i++) {
    if (pthread_create(&tid, NULL, produce, NULL) != SUCCESS) {
      perror("creating threads");
      return 1;
    }
  }

  if (pthread_join(tid, NULL) != SUCCESS) {  // prevent main() from terminating
    perror("joining threads");
    return 1;
  }
}
