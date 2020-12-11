#ifndef __QUEUE_H_
#define __QUEUE_H_

#include <stdalign.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdatomic.h>
#include <talloc.h>
#include <stdbool.h>

#define atomic_int64_t _Atomic(int64_t)

#define cas_incr(_store, _var)    atomic_compare_exchange_strong_explicit(&_store, &_var, _var + 1, memory_order_release, memory_order_relaxed)
#define cas_decr(_store, _var)    atomic_compare_exchange_strong_explicit(&_store, &_var, _var - 1, memory_order_release, memory_order_relaxed)
#define load(_var)           atomic_load_explicit(&_var, memory_order_relaxed)
#define aquire(_var)         atomic_load_explicit(&_var, memory_order_acquire)
#define store(_store, _var)  atomic_store_explicit(&_store, _var, memory_order_release);

struct atomic_queue_entry_t {
	alignas(128) void *data;
	atomic_int64_t seq;
};

struct atomic_queue_t {
	alignas(128) atomic_int64_t head;
	atomic_int64_t tail;
	int size;

	struct atomic_queue_entry_t entry[1];
};

struct atomic_queue_t*	atomic_queue_create(TALLOC_CTX *ctx, int size);
bool			atomic_queue_push(struct atomic_queue_t *aq, void *data);
bool			atomic_queue_pop(struct atomic_queue_t *aq, void **p_data);

#endif
