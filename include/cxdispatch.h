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

#if CX_CFG_USE_DISPATCH

typedef void (*dispatch_function_t)(void *);

#ifdef __cplusplus
extern "C" {
#endif
  void _dispatch_init(void);
  void cxDispatchAfter(systime_t delay, dispatch_function_t func, void *context);
#ifdef __cplusplus
}
#endif

#endif

#endif /* _CXDISPATCH_H_ */
