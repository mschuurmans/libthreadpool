#include <unistd.h>
#include <stdlib.h>
#include "log.h"
#include <stdbool.h>
#include "threads.h"

int main(int argc, char** argv)
{
	log_set_level(LOG_DEBUG);
	log_info("Started.");

	bool spawn = true;
	thread_pool_init(&spawn);

	int count = 0;
	for (;;) {
		struct REQUEST *r = malloc(sizeof(struct REQUEST));

		r->priority = 0;

		log_trace("Enqueing request.");
		request_enqueue(r);
		sleep(1);

		if (count++ > 20)
			break;
	}

	thread_pool_stop();

	return 0;
}
