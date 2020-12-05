#ifndef __THREADS_H_
#define __THREADS_H_

#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <semaphore.h>

#define SEMAPHORE_LOCKED	(0)

#define THREAD_RUNNING		(1)
#define THREAD_CANCELLED	(2)
#define THREAD_EXITED		(3)

struct REQUEST {
	int id;
};

struct THREAD_HANDLE {
	struct THREAD_HANDLE	*prev;
	struct THREAD_HANDLE	*next;
	pthread_t		pthread_id;
	int			thread_num;
	int			status;
	unsigned int		request_count;
	time_t			timestamp;
	struct REQUEST		*request;
};

struct THREAD_POOL {
	struct THREAD_HANDLE	*head;
	struct THREAD_HANDLE	*tail;

	uint32_t		total_threads;

	uint32_t		max_thread_num;
	uint32_t		start_threads;
	uint32_t		max_threads;
	uint32_t		min_spare_threads;
	uint32_t		max_spare_threads;
	uint32_t		max_request_per_thread;
	uint32_t		request_count;
	time_t			time_last_spawned;
	uint32_t		cleanup_delay;
	bool			stop_flag;
	bool			spawn_flag;

	pthread_mutex_t		wait_mutex;
	/*
	 * Threads wait for this semaphore for requests
	 * to enter the queue
	 */
	sem_t			semaphore;

	pthread_mutex_t		queue_mutex;
	uint32_t		active_threads; /* Protected by queue_mutex */
	uint32_t		exited_threads;
	uint32_t		num_queued;
	/*
	 * TODO Add a queue
	 */
};

int thread_pool_init(bool *spawn_flag);
void thread_pool_stop(void);

#endif
