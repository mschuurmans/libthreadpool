#include "threads.h"
#include "log.h"
#include <semaphore.h>
#include <stdlib.h>
#include <errno.h>
#include "queue.h"

static struct THREAD_POOL thread_pool;
static bool pool_initialized = false;

int request_enqueue(struct REQUEST *request)
{
	if (!atomic_queue_push(thread_pool.queue[request->priority], request)) {
		log_error("Failed inserting request into the queue");
		return 0;
	}
	
	thread_pool.request_count++;

	sem_post(&thread_pool.semaphore);

	return 1;
}

static int request_dequeue(struct REQUEST **prerequest)
{
	int i;
	struct REQUEST *request = NULL;

	for (i = 0; i < NUM_FIFOS; i++) {
		if (!atomic_queue_pop(thread_pool.queue[i], (void **) &request))
			continue;
		else
			break;
	}

	if (!request)
		return 0;

	*prerequest = request;

	CAS_INCR(thread_pool.active_threads);

	return 1;
}

void* thread_worker(void *arg)
{
	struct THREAD_HANDLE *handle = (struct THREAD_HANDLE*)arg;

	do {
		log_trace("Thread %d waiting to be assigned a request", handle->thread_num);

	re_wait:
		if (sem_wait(&thread_pool.semaphore) != 0) {
			if ((errno == EINTR || errno == EAGAIN)) {
				log_debug("Re-wait %d", handle->thread_num);
				goto re_wait;
			}

			log_error("Thread %d failed waiting for semaphore: %s: Exiting", handle->thread_num);
			break;
		}

		log_trace("Thread %d got semaphore", handle->thread_num);

		/*
		 * The server is exiting.
		 */
		if (thread_pool.stop_flag)
			break;

		if (!request_dequeue(&handle->request)) 
			continue;

		handle->request_count++;

		log_debug("thread %d Handling request", handle->thread_num);

		/* does this work?? */
		free(handle->request);

		handle->request = NULL;
	} while(handle->status != THREAD_CANCELLED);

	log_debug("Thread %d exiting...", handle->thread_num);

	pthread_mutex_lock(&thread_pool.queue_mutex);
	thread_pool.exited_threads++;
	pthread_mutex_unlock(&thread_pool.queue_mutex);

	handle->request = NULL;
	handle->status = THREAD_EXITED;

	return NULL;
}

/*
 * Spawns a new thread and adds it in the thread pool.
 */
static struct THREAD_HANDLE *thread_spawn(time_t now)
{
	struct THREAD_HANDLE *handle;
	int rcode;

	if (thread_pool.total_threads >= thread_pool.max_threads) {
		log_debug("Failed to spawn thread. Maximum number of threads (%d) already running.", thread_pool.max_threads);
		return NULL;
	}

	handle = malloc(sizeof(struct THREAD_HANDLE));
	memset(handle, 0, sizeof(struct THREAD_HANDLE));
	handle->prev = NULL;
	handle->next = NULL;
	handle->thread_num = thread_pool.max_thread_num++;
	handle->request_count = 0;
	handle->status = THREAD_RUNNING;
	handle->timestamp = time(NULL);

	rcode = pthread_create(&handle->pthread_id, 0, thread_worker, handle);
	if (rcode != 0) {
		free(handle);
		log_error("Thread create failed!");
		return NULL;
	}

	thread_pool.total_threads++;
	log_debug("Thread spawned new child %d, Total threads in pool: %d",
			handle->thread_num, thread_pool.total_threads);

	/*
	 * Add thread to tail of pool.
	 */
	if (thread_pool.tail) {
		thread_pool.tail->next = handle;
		handle->prev = thread_pool.tail;
		thread_pool.tail = handle;
	} else {
		thread_pool.head = thread_pool.tail = handle;
	}

	thread_pool.time_last_spawned = now;

	return handle;
}

static void thread_delete(struct THREAD_HANDLE *handle)
{
	struct THREAD_HANDLE *prev;
	struct THREAD_HANDLE *next;

	log_debug("Deleting thread %d", handle->thread_num);

	prev = handle->prev;
	next = handle->next;

	thread_pool.total_threads--;

	if (prev == NULL)
		thread_pool.head = next;
	else
		prev->next = next;


	if (next == NULL)
		thread_pool.tail = prev;
	else
		next->prev = prev;

	free(handle);
}

int thread_pool_init(bool *spawn_flag)
{
	uint32_t	i;
	int		rcode;
	time_t		now;

	now = time(NULL);

	memset(&thread_pool, 0, sizeof(struct THREAD_POOL));
	thread_pool.head = NULL;
	thread_pool.tail = NULL;
	thread_pool.total_threads = 0;
	thread_pool.max_thread_num = 1;
	thread_pool.cleanup_delay = 5;
	thread_pool.stop_flag = false;
	thread_pool.spawn_flag = *spawn_flag;
	thread_pool.max_queue_size = 10;

	thread_pool.start_threads = 3;
	thread_pool.max_threads = 5;

	if (!*spawn_flag)
		return 0;

	if ((pthread_mutex_init(&thread_pool.wait_mutex, NULL) != 0)) {
		log_error("FATAL: Failed to initialize wait mutex");

		return -1;
	}

	memset(&thread_pool.semaphore, 0, sizeof(thread_pool.semaphore));
	rcode = sem_init(&thread_pool.semaphore, 0, SEMAPHORE_LOCKED);
	if (rcode != 0) {
		log_error("FATAL: Failed to initialize semaphore");
		return -1;
	}

	for (i = 0; i < NUM_FIFOS; i++) {
		thread_pool.queue[i] = atomic_queue_create(NULL, thread_pool.max_queue_size);
		if (!thread_pool.queue[i]) {
			log_error("FATAL: Failed to set up request fifo");
			return -1;
		}
	}

	for (i = 0; i < thread_pool.start_threads; i++) {
		if (thread_spawn(now) == NULL) {
			return -1;
		}
	}

	log_debug("Thread pool initialized");
	pool_initialized = true;
	return 0;
}

void thread_pool_stop(void)
{
	int total_threads;
	int i;
	struct THREAD_HANDLE *handle;
	struct THREAD_HANDLE *next;

	if (!pool_initialized)
		return;

	thread_pool.stop_flag = true;

	/*
	 * Wakeup all thread to make them see stop flag.
	 */
	total_threads = thread_pool.total_threads;
	for (i = 0; i != total_threads; i++) {
		sem_post(&thread_pool.semaphore);
	}

	for (handle = thread_pool.head; handle; handle = next) {
		next = handle->next;
		pthread_join(handle->pthread_id, NULL);
		thread_delete(handle);
	}

	for (i = 0; i < NUM_FIFOS; i++) {
		talloc_free(thread_pool.queue[i]);
	}
}
