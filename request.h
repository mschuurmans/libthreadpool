#ifndef __REQUEST_H_
#define __REQUEST_H_

enum REQUEST_STATE {
	REQUEST_QUEUED = 1,
	REQUEST_PROCESSING,
	REQUEST_DONE
};

struct REQUEST {
	/*
	 * Identifier of the request. This will be set when
	 * the request is enqueued and is the request number 
	 * count for the thread pool at that given moment.
	 */
	int			id;

	/*
	 * Priority is used to determen on which fifo the 
	 * request is placed. The 0 will be checked first
	 * and then till NUM_FIFOS
	 */
	int			priority;

	/*
	 * State which the current request is in.
	 */
	enum REQUEST_STATE	state;

	/*
	 * Callback method to handle the request. This method
	 * will be called from the worker thread.
	 */
	int (*process)(struct REQUEST *request);
};

#endif
