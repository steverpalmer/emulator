/*
 * screen.h
 *
 *  Created on: 2 Apr 2012
 *      Author: steve
 */

#ifndef SCREEN_H_
#define SCREEN_H_

#include <log4c.h>

#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct My_Scr *Scr;

extern void        scr_init   ( void );
extern Scr         scr_new    ( float scale );
extern bool    scr_del    ( Scr scr );
extern void        scr_deinit ( void );
extern void        scr_dump   ( log4c_category_t *log, Scr scr );

extern void        scr_handle     ( Scr scr, int code );
extern byte     *scr_ram        ( Scr scr );
extern bool    scr_is_refresh ( Scr scr );

#ifdef __cplusplus
}
#endif

#endif /* SCREEN_H_ */
