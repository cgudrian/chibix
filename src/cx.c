/*
 * cx.c
 *
 *  Created on: 15.11.2014
 *      Author: christian
 */

#include "cx.h"

void cxInit(void) {
#if CX_CFG_USE_DISPATCH
  _dispatch_init();
#endif
}
