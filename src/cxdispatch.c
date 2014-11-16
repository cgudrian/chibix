/*
 * ChibiX - Copyright (C) 2014 Christian Gudrian
 *
 * This file is part of ChibiX.
 *
 * ChibiX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ChibiX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ChibiX.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "cx.h"

#if CX_CFG_USE_DISPATCH

typedef struct dispatch_item dispatch_item_t;

struct dispatch_item {
	systime_t xtime;
	dispatch_function_t func;
	void *context;
	dispatch_item_t *next;
};

static memory_pool_t item_pool;
static dispatch_item_t *queue_head;
static condition_variable_t item_has_been_queued;
static mutex_t queue_lock;

static void enqueue(dispatch_item_t *new_item) {
	dispatch_item_t *prev = NULL;
	dispatch_item_t *next = queue_head;

	// find a place for the new item in the queue
	while (next && (next->xtime <= new_item->xtime)) {
		prev = next;
		next = next->next;
	}

	// link the items together
	new_item->next = next;
	if (prev)
		prev->next = new_item;
	else
		// the new item becomes the first one
		queue_head = new_item;
}

static dispatch_item_t *dequeueFirstItem(void) {
	dispatch_item_t *entry = queue_head;

	if (queue_head)
		queue_head = queue_head->next;
	return entry;
}

static dispatch_item_t *createItem(systime_t xtime, dispatch_function_t func, void *context) {
	dispatch_item_t *item = chPoolAlloc(&item_pool);

	chDbgAssert(item != NULL, "could not allocate item");

	item->xtime = xtime;
	item->func = func;
	item->context = context;
	item->next = NULL;

	return item;
}

static inline void freeItem(dispatch_item_t *item) {
	chPoolFree(&item_pool, item);
}

void cxDispatchAfter(systime_t delay, dispatch_function_t func, void *context) {
	dispatch_item_t *item;
	systime_t xtime;

	xtime = chVTGetSystemTime() + delay;
	item = createItem(xtime, func, context);

	chMtxLock(&queue_lock);
	enqueue(item);
	chMtxUnlock(&queue_lock);

	// signal the dispatcher thread
	chCondSignal(&item_has_been_queued);
}

static inline systime_t delayForFirstItem(void) {

	chDbgAssert(queue_head != NULL, "queue is empty");
	return queue_head->xtime - chVTGetSystemTime();
}

static dispatch_item_t *waitForItem(void) {
	int32_t delay;
	dispatch_item_t *item;

	chMtxLock(&queue_lock);
	if (queue_head == NULL)
		// the queue is empty – sleep until an item gets queued
		chCondWait(&item_has_been_queued);
	delay = delayForFirstItem();
	while (delay > 0) {
		// the first item in the queue is not yet due for execution,
		// so wait until it is or another item has been queued
		if (chCondWaitTimeout(&item_has_been_queued, delay) == MSG_TIMEOUT)
			chMtxLock(&queue_lock);
		// update the delay (the first item might have been replaced)
		delay = delayForFirstItem();
	}

	// the first item is now due for execution
	item = dequeueFirstItem();
	chMtxUnlock(&queue_lock);

	return item;
}

static THD_WORKING_AREA(waDispatcher, CX_CFG_DISPATCH_WA_SIZE);
static THD_FUNCTION(dispatcher, arg) {
	(void) arg;
	dispatch_item_t *item;
	dispatch_function_t func;
	void *context;

	chRegSetThreadName("dispatcher");

	while (true) {
		item = waitForItem();

		// copy the necessary information so that the item can be
		// put back into the memory pool as soon as possible
		func = item->func;
		context = item->context;
		freeItem(item);

		// call the function
		func(context);
	}
	return 0;
}

void _dispatch_init(void) {

	chCondObjectInit(&item_has_been_queued);
	chPoolObjectInit(&item_pool, sizeof(dispatch_item_t), &chCoreAlloc);
	chMtxObjectInit(&queue_lock);
	chThdCreateStatic(waDispatcher, sizeof(waDispatcher), NORMALPRIO, dispatcher, NULL);
}

#endif
