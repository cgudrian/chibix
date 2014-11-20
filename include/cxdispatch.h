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

#ifndef _CXDISPATCH_H_
#define _CXDISPATCH_H_

#if CX_CFG_USE_DISPATCH_QUEUES

/**
 * Type of a dispatchable function.
 */
typedef void (*dispatch_function_t)( void * );

typedef struct dispatch_item dispatch_item_t;

struct dispatch_item {
	systime_t xtime;
	dispatch_function_t func;
	void *context;
	dispatch_item_t *next;
};

typedef struct {
	memory_pool_t item_pool;
	dispatch_item_t *queue_head;
	monitor_t monitor;
	tprio_t priority;
} dispatch_queue_t;

/**
 * Registers the given function for immediate execution.
 *
 * It is a shorthand for @c cxDispatchAfter(0, func, context)
 */
#define cxDispatch( queue, func, context ) cxDispatchAfter( (queue), 0, (func), (context) )

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes the dispatch subsystem.
 */
void _dispatch_init( void );

/**
 * Initializes a dispatch queue.
 *
 * This function initializes the provided dispatch queue and starts
 * a first worker thread.
 *
 * @param dq
 *      a pointer to a @c dispatch_queue_t structure
 * @param wsp
 *      a pointer to the working area for the worker thread
 * @param ws_size
 *      the size of the working area
 * @param thd_prio
 *      the priority of the worker thread
 */
void cxDispQueueObjectInit( dispatch_queue_t *dq, void *wsp, size_t ws_size, tprio_t thd_prio );

/**
 * Registers the given function for later execution and returns immediately.
 *
 * The provided function is guaranteed to not be executed before @c delay has
 * passed. However there might be an arbitrary large amount of additional
 * delay depending on the current state of the system.
 *
 * Once @c delay has passed @c func is executed by a worker thread.
 *
 * @c func may be put on the dispatch queue multiple times.
 *
 * @param dq
 *      the dispatch queue @c func should execute on.
 * @param delay
 *      the delay after which @c func should be executed.
 * @param func
 *      the function to be executed. May not be NULL.
 * @param context
 *      a user defined context that gets passed to @c func. May be NULL.
 */
void cxDispatchAfter( dispatch_queue_t *dq, systime_t delay, dispatch_function_t func, void *context );

/**
 * Adds another worker thread to the provided dispatch queue.
 *
 * @param dq
 *      the dispatch queue the thread should be added to
 * @param wsp
 *      the working area for the new thread
 * @param ws_size
 *      the size of the working area for the new thread
 */
void cxDispatchAddThread( dispatch_queue_t *dq, void *wsp, size_t ws_size );

#ifdef __cplusplus
}
#endif

#endif /* CX_CFG_USE_DISPATCH_QUEUES */

#endif /* _CXDISPATCH_H_ */
