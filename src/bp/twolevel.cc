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

extern "C" {
#include "bp/bp.param.h"
#include "core.param.h"
#include "globals/assert.h"
#include "statistics.h"
}

#define DEBUG(proc_id, args...) _DEBUG(proc_id, DEBUG_BP_DIR, ##args)

/**************************************************/
/* Two level Branch Predictor Data Structures*/

#define _i 8
#define _j 8
#define _k 16

namespace {
  long long* global_history_register;
  uns8** pattern_history_table;
} 

/**************************************************/

void bp_twolevel_timestamp(Op* op) {}
void bp_twolevel_recover(Recovery_Info* info) {}
void bp_twolevel_spec_update(Op* op) {}
void bp_twolevel_retire(Op* op) {}
uns8 bp_twolevel_full(uns proc_id) { return 0; }


void bp_twolevel_init() {
  /**************************************************/
  /* Initialize pattern_history_table*/
  int rows = pow(2, _k);
  int cols = pow(2, _j);
  int global_size = pow(2, _i);

  // Allocate memory for rows
  pattern_history_table = (uns8**)malloc(rows * sizeof(uns8*));

  // Allocate memory for each row's columns
  for (int i = 0; i < rows; ++i) {
    pattern_history_table[i] = (uns8*)malloc(cols * sizeof(uns8));
  }

  // Initialize the table with zeros
  for (int i = 0; i < rows; ++i) {
    for (int j = 0; j < cols; ++j) {
      pattern_history_table[i][j] = 1; // not taken
    }
  }
  /**************************************************/
  /* Initialize global_history_register*/
  global_history_register = (long long*)malloc(global_size * sizeof(long long));

  // Initialize global_history_register with zeros
  for (int idx = 0; idx < global_size; ++idx) {
    global_history_register[idx] = 0;
  }
  /**************************************************/
}

uns8 bp_twolevel_pred(Op* op) {
  /**************************************************/
  /* Make the prediction */
  const Addr address = op->oracle_info.pred_addr;
  unsigned int ghrIndex = address & ~(~0 << _i);
  unsigned int phtIndex = address & ~(~0 << _j);
	unsigned int ghr = global_history_register[ghrIndex];
  
  //Predict taken if the saturating counter value is greater than or equal to 2
  const uns8 pred = (pattern_history_table[phtIndex][ghr] >= 2);

  return pred;
  /**************************************************/
}

void bp_twolevel_update(Op* op) {
  if(op->table_info->cf_type != CF_CBR) {
    // If op is not a conditional branch, we do not interact with gshare.
    return;
  }

  /**************************************************/
  /* Update pattern_history_table*/
  const Addr address = op->oracle_info.pred_addr;
  unsigned int ghrIndex = address & ~(~0 << _i);
  unsigned int phtIndex = address & ~(~0 << _j);
	unsigned int ghr = global_history_register[ghrIndex];

  bool actual_outcome = (op->oracle_info.dir == TAKEN);
  
  //Lambda funtion to update the pattern history table
  if (actual_outcome) {
    if (pattern_history_table[phtIndex][ghr] < 3) {
      pattern_history_table[phtIndex][ghr] += 1;
    }
  } else {
    if (pattern_history_table[phtIndex][ghr] > 0) {
      pattern_history_table[phtIndex][ghr] -= 1;
    }
  }

  /* Update global_history_register*/
  global_history_register[ghrIndex] = ((global_history_register[ghrIndex] << 1) | actual_outcome) & ((1 << _k) - 1);
  
  /**************************************************/
}
