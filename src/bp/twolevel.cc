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

#include "twolevel.h"

#include <math.h>
#include <iostream>
#include <string>

extern "C" {
#include "bp/bp.param.h"
#include "core.param.h"
#include "globals/assert.h"
#include "statistics.h"
}

#define DEBUG(proc_id, args...) _DEBUG(proc_id, DEBUG_BP_DIR, ##args)

/**************************************************/
/* Two level Branch Predictor Data Structures*/

#define _i PHT_SIZE
#define _k GHR_SIZE

namespace {
  long long global_history_register;
  uns8* pattern_history_table;
  long long* global_history_register_table;
  uns8** pattern_history_table_2d;
  const std::string TWOLEVEL_CONFIG = "Pag";
} 

/**************************************************/

void bp_twolevel_timestamp(Op* op) {}
void bp_twolevel_recover(Recovery_Info* info) {}
void bp_twolevel_spec_update(Op* op) {}
void bp_twolevel_retire(Op* op) {}
uns8 bp_twolevel_full(uns proc_id) { return 0; }

/**************************************************/
void bp_twolevel_init() {
  if(TWOLEVEL_CONFIG == "Gag"){
    bp_twolevel_init_gag();
  }
  if(TWOLEVEL_CONFIG == "Gap"){
    bp_twolevel_init_gap();
  }
  if(TWOLEVEL_CONFIG == "Pag"){
    bp_twolevel_init_pag();
  }
  if(TWOLEVEL_CONFIG == "Pap"){
    bp_twolevel_init_pap();
  }
}
/**************************************************/
uns8 bp_twolevel_pred(Op* op) {
  if(TWOLEVEL_CONFIG == "Gag"){
    return bp_twolevel_pred_gag(op);
  }
  if(TWOLEVEL_CONFIG == "Gap"){
    return bp_twolevel_pred_gap(op);
  }
  if(TWOLEVEL_CONFIG == "Pag"){
    return bp_twolevel_pred_pag(op);
  }
  if(TWOLEVEL_CONFIG == "Pap"){
    return bp_twolevel_pred_pap(op);
  }
  return 0;
}
/**************************************************/
void bp_twolevel_update(Op* op) {
  if(op->table_info->cf_type != CF_CBR) {
    // If op is not a conditional branch, we do not interact with gshare.
    return;
  }
  if(TWOLEVEL_CONFIG == "Gag"){
    bp_twolevel_update_gag(op);
  }
  if(TWOLEVEL_CONFIG == "Gap"){
    bp_twolevel_update_gap(op);
  }
  if(TWOLEVEL_CONFIG == "Pag"){
    bp_twolevel_update_pag(op);
  }
  if(TWOLEVEL_CONFIG == "Pap"){
    bp_twolevel_update_pap(op);
  }
}

/**************************************************/
void bp_twolevel_init_gag(){
  int rows = pow(2, _k);
  pattern_history_table = (uns8*)malloc(rows * sizeof(uns8));

  for (int i = 0; i < rows; ++i) {
    pattern_history_table[i] = 1; // not taken
  }
  global_history_register = 0;
}
/**************************************************/
void bp_twolevel_init_gap(){
  int rows = pow(2, _i);
  int cols = pow(2, _k);

  pattern_history_table_2d = (uns8**)malloc(rows * sizeof(uns8*));

  for (int i = 0; i < rows; ++i) {
    pattern_history_table_2d[i] = (uns8*)malloc(cols * sizeof(uns8));
  }

  for (int i = 0; i < rows; ++i) {
    for (int j = 0; j < cols; ++j) {
      pattern_history_table_2d[i][j] = 1; // not taken
    }
  }
  global_history_register = 0;
}
/**************************************************/
void bp_twolevel_init_pag(){
  int rows = pow(2, _k);
  int global_size = pow(2, _i);
  pattern_history_table = (uns8*)malloc(rows * sizeof(uns8));

  for (int i = 0; i < rows; ++i) {
    pattern_history_table[i] = 1; // not taken
  }
  global_history_register_table = (long long*)malloc(global_size * sizeof(long long));
  
  for (int idx = 0; idx < global_size; ++idx) {
    global_history_register_table[idx] = 0;
  }

}
/**************************************************/
void bp_twolevel_init_pap(){
  int rows = pow(2, _i);
  int cols = pow(2, _k);
  int global_size = pow(2, _i);

  pattern_history_table_2d = (uns8**)malloc(rows * sizeof(uns8*));

  for (int i = 0; i < rows; ++i) {
    pattern_history_table_2d[i] = (uns8*)malloc(cols * sizeof(uns8));
  }

  for (int i = 0; i < rows; ++i) {
    for (int j = 0; j < cols; ++j) {
      pattern_history_table_2d[i][j] = 1; // not taken
    }
  }
  
  global_history_register_table = (long long*)malloc(global_size * sizeof(long long));
  
  for (int idx = 0; idx < global_size; ++idx) {
    global_history_register_table[idx] = 0;
  }
}
/**************************************************/
uns8 bp_twolevel_pred_gag(Op* op){
  const uns8 pred = (pattern_history_table[global_history_register] >= 2);
  return pred;
}
/**************************************************/
uns8 bp_twolevel_pred_gap(Op* op){
  const Addr address = op->oracle_info.pred_addr;
  unsigned int phtIndex = address & ~(~0 << _i);

  const uns8 pred = (pattern_history_table_2d[phtIndex][global_history_register] >= 2);
  return pred;
}
/**************************************************/
uns8 bp_twolevel_pred_pag(Op* op){
  const Addr address = op->oracle_info.pred_addr;
  unsigned int ghrIndex = address & ~(~0 << _i);
	unsigned int ghr = global_history_register_table[ghrIndex];

  const uns8 pred = (pattern_history_table[ghr] >= 2);
  return pred;
}
/**************************************************/
uns8 bp_twolevel_pred_pap(Op* op){
  const Addr address = op->oracle_info.pred_addr;
  unsigned int ghrIndex = address & ~(~0 << _i);
  unsigned int phtIndex = address & ~(~0 << _i);
	unsigned int ghr = global_history_register_table[ghrIndex];

  const uns8 pred = (pattern_history_table_2d[phtIndex][ghr] >= 2);
  return pred;
}
/**************************************************/
void bp_twolevel_update_gag(Op* op){
  bool actual_outcome = (op->oracle_info.dir == TAKEN);
  
  if (actual_outcome) {
    if (pattern_history_table[global_history_register] < 3) {
      pattern_history_table[global_history_register] += 1;
    }
  } else {
    if (pattern_history_table[global_history_register] > 0) {
      pattern_history_table[global_history_register] -= 1;
    }
  }

  global_history_register = ((global_history_register << 1) | actual_outcome) & ((1 << _k) - 1);
}
/**************************************************/
void bp_twolevel_update_gap(Op* op){
  const Addr address = op->oracle_info.pred_addr;
  unsigned int phtIndex = address & ~(~0 << _i);

  bool actual_outcome = (op->oracle_info.dir == TAKEN);
  
  if (actual_outcome) {
    if (pattern_history_table_2d[phtIndex][global_history_register] < 3) {
      pattern_history_table_2d[phtIndex][global_history_register] += 1;
    }
  } else {
    if (pattern_history_table_2d[phtIndex][global_history_register] > 0) {
      pattern_history_table_2d[phtIndex][global_history_register] -= 1;
    }
  }

  global_history_register = ((global_history_register << 1) | actual_outcome) & ((1 << _k) - 1);
}
/**************************************************/
void bp_twolevel_update_pag(Op* op){
  const Addr address = op->oracle_info.pred_addr;
  unsigned int ghrIndex = address & ~(~0 << _i);
  
  bool actual_outcome = (op->oracle_info.dir == TAKEN);

  if (actual_outcome) {
    if (pattern_history_table[global_history_register_table[ghrIndex]] < 3) {
      pattern_history_table[global_history_register_table[ghrIndex]] += 1;
    }
  } else {
    if (pattern_history_table[global_history_register_table[ghrIndex]] > 0) {
      pattern_history_table[global_history_register_table[ghrIndex]] -= 1;
    }
  }

  global_history_register_table[ghrIndex] = ((global_history_register_table[ghrIndex] << 1) | actual_outcome) & ((1 << _k) - 1);
}
/**************************************************/
void bp_twolevel_update_pap(Op* op){
  const Addr address = op->oracle_info.pred_addr;
  unsigned int ghrIndex = address & ~(~0 << _i);
  unsigned int phtIndex = address & ~(~0 << _i);
	unsigned int ghr = global_history_register_table[ghrIndex];

  bool actual_outcome = (op->oracle_info.dir == TAKEN);

  if (actual_outcome) {
    if (pattern_history_table_2d[phtIndex][ghr] < 3) {
      pattern_history_table_2d[phtIndex][ghr] += 1;
    }
  } else {
    if (pattern_history_table_2d[phtIndex][ghr] > 0) {
      pattern_history_table_2d[phtIndex][ghr] -= 1;
    }
  }

  global_history_register_table[ghrIndex] = ((global_history_register_table[ghrIndex] << 1) | actual_outcome) & ((1 << _k) - 1);
}
/**************************************************/