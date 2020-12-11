#include "queue.h"
#include <talloc.h>
#include <stdbool.h>

struct atomic_queue_t *atomic_queue_create(TALLOC_CTX *ctx, int size)
{
	int i;
	int64_t seq;
	struct atomic_queue_t *aq;

	if (size <= 0)
		return NULL;

	aq = talloc_size(ctx, sizeof(*aq) + (size - 1) * sizeof(aq->entry[0]));

	if (!aq)
		return NULL;

	talloc_set_name(aq, "atomic_queue_t");

	for (i = 0; i < size; i++) {
		seq = i;

		aq->entry[i].data = NULL;
		store(aq->entry[i].seq, seq);
	}

	aq->size = size;

	store(aq->head, 0);
	store(aq->tail, 0);
	atomic_thread_fence(memory_order_seq_cst);

	return aq;
}

bool atomic_queue_push(struct atomic_queue_t *aq, void *data)
{
	int64_t head;
	struct atomic_queue_entry_t *entry;

	if (!data)
		return false;

	head = load(aq->head);

	for (;;) {
		int64_t seq, diff;

		entry = &aq->entry[ head % aq->size ];
		seq = aquire(entry->seq);
		diff = (seq - head);

		/*
		 * Head is larger then current entry, queue is full
		 */
		if (diff < 0) {
			return false;
		}


		/*
		 * This entry is written to. Get new head pointer and continue
		 */
		if (diff > 0) {
			head = load(aq->head);
			continue;
		}

		if (cas_incr(aq->head, head)) {
			break;
		}
	}

	entry->data = data;
	store(entry->seq, head + 1);

	return true;
}

bool atomic_queue_pop(struct atomic_queue_t *aq, void **p_data)
{
	int64_t tail, seq;
	struct atomic_queue_entry_t *entry;

	if (!p_data)
		return false;

	tail = load(aq->tail);

	for (;;) {
		int64_t diff;

		entry = &aq->entry[ tail % aq->size ];
		seq = aquire(entry->seq);

		diff = (seq - (tail + 1));

		if (diff < 0)
			return false;

		if (diff > 0) {
			tail = load(aq->tail);
			continue;
		}

		if (cas_incr(aq->tail, tail)) 
			break;
	}

	*p_data = entry->data;

	seq = tail + aq->size;
	store(entry->seq, seq);

	return true;
}
