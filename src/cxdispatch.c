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

/**
 * Puts an item in the queue.
 */
static void enqueue( dispatch_item_t *new_item ) {
	dispatch_item_t *prev = NULL;
	dispatch_item_t *next = queue_head;

	// find a place for the new item in the queue
	while( next && (next->xtime <= new_item->xtime) ) {
		prev = next;
		next = next->next;
	}

	// link the items together
	new_item->next = next;
	if( prev )
		prev->next = new_item;
	else
		// the new item becomes the first one
		queue_head = new_item;
}

/**
 * Dequeues the first item and returns it.
 */
static inline dispatch_item_t *dequeue_first( void ) {
	dispatch_item_t *entry = queue_head;

	if( queue_head )
		queue_head = queue_head->next;
	return entry;
}

/**
 * Allocates an item from the memory pool and initializes it.
 *
 * This function returns NULL, if the memory pool is empty.
 */
static dispatch_item_t *create_item( systime_t xtime, dispatch_function_t func, void *context ) {
	dispatch_item_t *item = chPoolAlloc( &item_pool );

	if( item ) {
		item->xtime = xtime;
		item->func = func;
		item->context = context;
		item->next = NULL;
	}

	return item;
}

/**
 * Puts an item back into the memory pool.
 */
static inline void free_item( dispatch_item_t *item ) {
	chPoolFree( &item_pool, item );
}

void cxDispatchAfter( systime_t delay, dispatch_function_t func, void *context ) {
	dispatch_item_t *item;

	chDbgCheck( func != NULL );

	item = create_item( chVTGetSystemTime() + delay, func, context );
	if( item ) {
		chMtxLock( &queue_lock );
		enqueue( item );
		chMtxUnlock( &queue_lock );

		// signal a waiting dispatcher thread
		chCondSignal( &item_has_been_queued );
	}
}

/**
 * Returns the delay for the first item in the queue.
 *
 * If the queue is empty this function waits until an item has been queued.
 * This function may return a negative value which means, that the
 * first item in the queue is overdue for execution.
 */
static inline int32_t first_delay( void ) {

	if( queue_head == NULL )
		// the queue is currently empty - wait until an item has been queued
		chCondWait( &item_has_been_queued );
	chDbgAssert( queue_head != NULL, "queue still empty" );
	return queue_head->xtime - chVTGetSystemTime();
}

/**
 * Waits until the first item becomes due for execution an returns it.
 */
static dispatch_item_t *next_item( void ) {
	int32_t delay;
	dispatch_item_t *item;

	chMtxLock( &queue_lock );
	while( (delay = first_delay()) > 0 ) {
		// the first item in the queue is not yet due for execution,
		// so wait until it is or another item has been queued
		if( chCondWaitTimeout( &item_has_been_queued, delay ) == MSG_TIMEOUT )
			// re-acquire the lock after a timeout
			chMtxLock( &queue_lock );
	}
	// the first item is now due for execution
	item = dequeue_first();
	chMtxUnlock( &queue_lock );

	return item;
}

static THD_WORKING_AREA(wa_dispatcher, CX_CFG_DISPATCH_WA_SIZE);
static THD_FUNCTION(dispatcher, arg) {
	(void)arg;
	dispatch_item_t *item;
	dispatch_function_t func;
	void *context;

	chRegSetThreadName( "dispatcher" );

	while( true ) {
		item = next_item();

		// copy the necessary information so that the item can be
		// put back into the memory pool as soon as possible
		func = item->func;
		context = item->context;
		free_item( item );

		// call the function
		func( context );
	}
	return 0;
}

#if CX_CFG_DISPATCH_QUEUE_SIZE > 0
static dispatch_item_t pool_items[CX_CFG_DISPATCH_QUEUE_SIZE];
#endif

void _dispatch_init( void ) {

	chCondObjectInit( &item_has_been_queued );
#if CX_CFG_DISPATCH_QUEUE_SIZE == 0
	chPoolObjectInit( &item_pool, sizeof(dispatch_item_t), &chCoreAlloc );
#else
	chPoolObjectInit( &item_pool, sizeof(dispatch_item_t), NULL );
	chPoolLoadArray( &item_pool, pool_items, CX_CFG_DISPATCH_QUEUE_SIZE );
#endif
	chMtxObjectInit( &queue_lock );
	chThdCreateStatic( wa_dispatcher, sizeof(wa_dispatcher), NORMALPRIO, dispatcher, NULL );
}

#endif /* CX_CFG_USE_DISPATCH */
