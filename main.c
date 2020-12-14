#include <unistd.h>
#include <stdlib.h>
#include "log.h"
#include <stdbool.h>
#include "threads.h"

int process(struct REQUEST *request)
{
	request->state = REQUEST_PROCESSING;

	request->state = REQUEST_DONE;
	return 0;
}

int main(int argc, char** argv)
{
	log_set_level(LOG_DEBUG);
	log_info("Started.");

	bool spawn = true;
	thread_pool_init(&spawn);

	int count = 0;
	for (;;) {
		struct REQUEST *r = malloc(sizeof(struct REQUEST));
		r->process = &process;

		r->priority = 0;

		log_trace("Enqueing request.");
		request_enqueue(r);

		if (count++ > 165000)
			break;
	}

	sleep(20);

	thread_pool_stop();

	return 0;
}
