#ifndef __REQUEST_H_
#define __REQUEST_H_

enum REQUEST_STATE {
	REQUEST_QUEUED = 1,
	REQUEST_PROCESSING,
	REQUEST_DONE
};

struct REQUEST {
	int			id;
	int			priority;
	enum REQUEST_STATE	state;
	int (*process)(struct REQUEST *request);
};

#endif
