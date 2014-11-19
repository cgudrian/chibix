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

dispatch_queue_t normal_priority_queue;

/**
 * Puts an item in the queue.
 */
static void enqueue( dispatch_queue_t *dq, dispatch_item_t *new_item )
{
	dispatch_item_t *prev = NULL;
	dispatch_item_t *next = dq->queue_head;

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
		dq->queue_head = new_item;
}

/**
 * Dequeues the first item and returns it.
 */
static inline dispatch_item_t *dq_dequeue_first( dispatch_queue_t *dq )
{
	dispatch_item_t *entry = dq->queue_head;

	if( entry )
		dq->queue_head = entry->next;
	return entry;
}

/**
 * Allocates an item from the memory pool and initializes it.
 *
 * This function returns NULL, if the memory pool is empty.
 */
static dispatch_item_t *dq_create_item( dispatch_queue_t *dq,
                                        systime_t xtime,
                                        dispatch_function_t func, void *context )
{
	dispatch_item_t *item = chPoolAlloc( &dq->item_pool );

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
static inline void dq_free_item( dispatch_queue_t *dq, dispatch_item_t *item )
{
	chPoolFree( &dq->item_pool, item );
}

static inline void dq_lock( dispatch_queue_t *dq )
{
	cxMonitorLock( &dq->monitor );
}

static inline void dq_unlock( dispatch_queue_t *dq )
{
	cxMonitorUnlock( &dq->monitor );
}

static inline void dq_wait( dispatch_queue_t *dq )
{
	cxMonitorWait( &dq->monitor );
}

static inline msg_t dq_wait_timeout( dispatch_queue_t *dq, systime_t time )
{
	return cxMonitorWaitTimeout( &dq->monitor, time );
}

static inline void dq_notify( dispatch_queue_t *dq )
{
	cxMonitorNotify( &dq->monitor );
}

static inline bool dq_is_empty( dispatch_queue_t *dq )
{
	return dq->queue_head == NULL;
}

void cxDispatchAfter( dispatch_queue_t *dq, systime_t delay, dispatch_function_t func, void *context )
{
	dispatch_item_t *item;

	chDbgCheck( (dq != NULL) && (func != NULL) );

	item = dq_create_item( dq, chVTGetSystemTime() + delay, func, context );
	if( item ) {
		dq_lock( dq );
		enqueue( dq, item );
		dq_unlock( dq );

		// signal a waiting dispatcher thread
		dq_notify( dq );
	}
}

/**
 * Returns the delay for the first item in the queue.
 *
 * If the queue is empty this function waits until an item has been queued.
 * This function may return a negative value which means, that the
 * first item in the queue is overdue for execution.
 */
static inline int32_t dq_first_delay( dispatch_queue_t *dq )
{
	if( dq_is_empty( dq ) )
		// the queue is currently empty - wait until an item has been queued
		dq_wait( dq );
	chDbgAssert( !dq_is_empty( dq ), "queue is still empty" );
	return dq->queue_head->xtime - chVTGetSystemTime();
}

/**
 * Waits until the first item becomes due for execution an returns it.
 */
static dispatch_item_t *dq_next_item( dispatch_queue_t *dq )
{
	int32_t delay;
	dispatch_item_t *item;

	dq_lock( dq );
	while( (delay = dq_first_delay( dq )) > 0 )
		// the first item in the queue is not yet due for execution,
		// so wait until it is or another item has been queued
		dq_wait_timeout( dq, delay );

	// the first item is now due for execution
	item = dq_dequeue_first( dq );
	dq_unlock( dq );

	return item;
}

/**
 * The dispatcher thread.
 *
 * This thread waits for items to be put on the dispatch queue
 * that gets passed in as the argument. Once an item is available
 * and due for execution it is dequeued and executed.
 */
static THD_FUNCTION( dispatcher, dq ) {
	dispatch_item_t *item;
	dispatch_function_t func;
	void *context;

	chRegSetThreadName( "dispatcher" );

	while( true ) {
		item = dq_next_item( dq );

		// copy the necessary information so that the item can be
		// put back into the memory pool as soon as possible
		func = item->func;
		context = item->context;
		dq_free_item( dq, item );

		// call the function
		func( context );
	}
	return 0;
}

void cxDispQueueObjectInit( dispatch_queue_t *dq, void *wsp, size_t ws_size, tprio_t thd_prio )
{
	chDbgCheck( dq != NULL );

	dq->queue_head = NULL;
	cxMonitorObjectInit( &dq->monitor );
	chPoolObjectInit( &dq->item_pool, sizeof(dispatch_item_t), &chCoreAlloc );

	chThdCreateStatic( wsp, ws_size, thd_prio, &dispatcher, dq );
}

#if CX_CFG_DISPATCH_QUEUE_SIZE > 0
static dispatch_item_t pool_items[CX_CFG_DISPATCH_QUEUE_SIZE];
#endif

void _dispatch_init( void )
{
	static THD_WORKING_AREA( normal_pq_workarea, CX_CFG_DISPATCH_WA_SIZE );

	cxDispQueueObjectInit( &normal_priority_queue,
	                       &normal_pq_workarea, CX_CFG_DISPATCH_WA_SIZE,
	                       NORMALPRIO );
}

#endif /* CX_CFG_USE_DISPATCH */
