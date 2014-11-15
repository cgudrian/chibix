/*
 * cxdispatch.h
 *
 *  Created on: 15.11.2014
 *      Author: christian
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
