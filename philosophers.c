#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define SUCCESS 0

// global structs
// 5 philosophers with index 1 - 5
char *philos[] = {"", "Alice", "Bob", "Carol", "David", "Esan"};
pthread_mutex_t forks[5];  // 5 forks in a round

// the activity of ith philosopher
void *philo_act(void *intVal) {
	int seat = *(int *)intVal;
	srand(seat);  // init the random seed
	for (int round = 1, dinnerCnt = 0;; round++) {
		char *thisGuy = philos[seat];
		printf("[%d]/[%d] %s is thinking\n", dinnerCnt, round, thisGuy);
		// sleep for 0~1s randomly to prevent race
		usleep(rand() % 1000000);  // us
		printf("[%d]/[%d] %s becomes hungry\n", dinnerCnt, round, thisGuy);

		// try to get the right fork
		if (pthread_mutex_trylock(&forks[(seat + 1) % 5]) != SUCCESS) {
			continue;  // go back thinking
		}
		printf("[%d]/[%d] %s gets the right fork\n", dinnerCnt, round, thisGuy);
		// try to get the left fork
		if (pthread_mutex_trylock(&forks[seat - 1]) != SUCCESS) {
			(pthread_mutex_unlock(&forks[(seat + 1) % 5]));  // realease lock
			continue;                                        // go back thinking
		}
		printf("[%d]/[%d] %s gets the left fork\n", dinnerCnt, round, thisGuy);

		// if get both forks
		usleep(rand() % 1000000);  // us
		dinnerCnt++;
		printf("[%d]/[%d] %s finishes %dth dinner\n", dinnerCnt, round, thisGuy,
					 dinnerCnt);
		// release forks
		pthread_mutex_unlock(&forks[(seat + 1) % 5]);
		pthread_mutex_unlock(&forks[seat - 1]);
	}
}

int main() {
	// init the mutex locks
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	for (int i = 0; i < 5; i++) {
		pthread_mutex_init(&forks[i], &attr);
	}

	// invite philosophers
	int params[] = {0, 1, 2, 3, 4, 5};  // philos' seats num
	pthread_t tid;                      // thread id
	for (int i = 1; i <= 5; i++) {
		if (pthread_create(&tid, NULL, philo_act, (void *)&params[i]) != SUCCESS) {
			perror("creating thread");
			return 1;
		}
		printf("%s joined\n", philos[i]);
	}
	// join the last tid to prevent main() terminates
	pthread_join(tid, NULL);
}