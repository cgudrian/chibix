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

#ifndef _CXMONITOR_H_
#define _CXMONITOR_H_

#if CX_CFG_USE_MONITORS

typedef struct {
	mutex_t lock;
	condition_variable_t cond;
} monitor_t;

/**
 * Acquires the lock on the provided monitor.
 */
static inline void cxMonitorLock( monitor_t *monitor )
{
	chDbgCheck( monitor != NULL );

	chMtxLock( &monitor->lock );
}

/**
 * Releases the lock on the provided monitor.
 */
static inline void cxMonitorUnlock( monitor_t *monitor )
{
	chDbgCheck( monitor != NULL );

	chMtxUnlock( &monitor->lock );
}

/**
 * Notifies one thread that is currently waiting on this monitor.
 */
static inline void cxMonitorNotify( monitor_t *monitor )
{
	chDbgCheck( monitor != NULL );

	chCondSignal( &monitor->cond );
}

/**
 * Notifies all threads that are currently waiting on this monitor.
 */
static inline void cxMonitorNotifyAll( monitor_t *monitor )
{
	chDbgCheck( monitor != NULL );

	chCondBroadcast( &monitor->cond );
}

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes a monitor object.
 *
 * A monitor combines a mutex with a condition variable.
 */
void cxMonitorObjectInit( monitor_t *monitor );

/**
 * Releases the provided monitor's lock and waits until the monitor gets
 * notified.
 *
 * The invoking thread must hold the monitor's lock.
 */
msg_t cxMonitorWait( monitor_t *monitor );

/**
 * Releases the provided monitor's lock and waits until the monitor gets
 * notified or the provided timeout expires.
 *
 * In contrast to chCondWaitTimeout this function re-acquires the monitor's
 * lock when a timeout occurs and thus mimic the Java behavior.
 *
 * The invoking thread must hold the monitor's lock.
 */
msg_t cxMonitorWaitTimeout( monitor_t *monitor, systime_t time );

#ifdef __cplusplus
}
#endif

#endif /* CX_CFG_USE_MONITORS */

#endif /* _CXMONITOR_H_ */
