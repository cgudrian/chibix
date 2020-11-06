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

#if CX_CFG_USE_MONITORS

#if CH_DBG_SYSTEM_STATE_CHECK
static inline void cxMonitorCheckLock(monitor_t *monitor)
{
	if (chMtxGetNextMutexS() != &monitor->lock)
		chSysHalt("CX#1");
}
#else
#define cxMonitorCheckLock(m)
#endif

void cxMonitorObjectInit(monitor_t *monitor)
{
	chDbgCheck(monitor != NULL);

	chMtxObjectInit(&monitor->lock);
	chCondObjectInit(&monitor->cond);
}

msg_t cxMonitorWait(monitor_t *monitor)
{
	msg_t msg;

	chDbgCheck(monitor != NULL);

	chSysLock();
	cxMonitorCheckLock(monitor);
	msg = chCondWaitS(&monitor->cond);
	chSysUnlock();

	return msg;
}

#if CH_CFG_USE_CONDVARS_TIMEOUT
msg_t cxMonitorWaitTimeout(monitor_t *monitor, systime_t time)
{
	msg_t msg;

	chDbgCheck(monitor != NULL);

	chSysLock();
	cxMonitorCheckLock(monitor);
	msg = chCondWaitTimeoutS(&monitor->cond, time);
	if (msg == MSG_TIMEOUT)
		chMtxLockS(&monitor->lock);
	chSysUnlock();

	return msg;
}
#endif

#endif
