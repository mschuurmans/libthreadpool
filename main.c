#include <unistd.h>
#include "log.h"
#include <stdbool.h>
#include "threads.h"

int main(int argc, char** argv)
{
	log_info("Started.");

	bool spawn = true;
	thread_pool_init(&spawn);

	sleep(10);

	thread_pool_stop();

	return 0;
}
