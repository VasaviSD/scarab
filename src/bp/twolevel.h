/* Copyright 2020 HPS/SAFARI Research Groups
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __TWOLEVEL_H__
#define __TWOLEVEL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bp/bp.h"

/*************Interface to Scarab***************/
void bp_twolevel_init(void);
void bp_twolevel_init_gag(void);
void bp_twolevel_init_gap(void);
void bp_twolevel_init_pag(void);
void bp_twolevel_init_pap(void);
void bp_twolevel_timestamp(Op*);
uns8 bp_twolevel_pred(Op*);
uns8 bp_twolevel_pred_gag(Op*);
uns8 bp_twolevel_pred_gap(Op*);
uns8 bp_twolevel_pred_pag(Op*);
uns8 bp_twolevel_pred_pap(Op*);
void bp_twolevel_spec_update(Op*);
void bp_twolevel_update(Op*);
void bp_twolevel_update_gag(Op*);
void bp_twolevel_update_gap(Op*);
void bp_twolevel_update_pag(Op*);
void bp_twolevel_update_pap(Op*);
void bp_twolevel_retire(Op*);
void bp_twolevel_recover(Recovery_Info*);
uns8 bp_twolevel_full(uns);

#ifdef __cplusplus
}
#endif


#endif  // __TWOLEVEL_H__
