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
#include <vector>
#include <algorithm> 

extern "C" {
#include "bp/bp.param.h"
#include "core.param.h"
#include "globals/assert.h"
#include "statistics.h"
}

#define DEBUG(proc_id, args...) _DEBUG(proc_id, DEBUG_BP_DIR, ##args)

std::string HRT_TYPE = "AHRT";

/**************************************************/
/* Two level Branch Predictor Data Structures*/

namespace nm {
  int AHRT_ASSOC = 4; // 4-way set associative cache
  struct CacheEntry {
      Addr tag;
      long long history;
      bool valid;
      int64_t timestamp;
      CacheEntry() : tag(0), history(0), valid(false), timestamp(0) {}
  };

  struct CacheSet {
      std::vector<CacheEntry> entries;
      CacheSet() : entries(AHRT_ASSOC) {}
  };

  
  long long* hash_hrt;
  std::vector<CacheSet> cache(0);
  uns8* pattern_history_table;
  int64_t currentTime;

/**************************************************/
unsigned int hash(Addr address) {
    return address % HRT_SIZE;
}

/**************************************************/
int64_t get_current_time() {
    return ++currentTime;  // Increment and return the new time
}
/*************Cache Functions*********************/

unsigned int compute_set_index(Addr address) {
    int AHRT_SETS = HRT_SIZE/AHRT_ASSOC;
    unsigned index_length = static_cast<unsigned>(std::log2(AHRT_SETS));
    return address & ((1 << index_length) - 1);
}
/**************************************************/
Addr compute_tag(Addr address) {
    int AHRT_SETS = HRT_SIZE/AHRT_ASSOC;
    unsigned index_length = static_cast<unsigned>(std::log2(AHRT_SETS));
    return address >> index_length;
}
/**************************************************/
int check_hit(CacheSet& set, Addr tag) {
    for (int i = 0; i < AHRT_ASSOC; ++i) {
        if (set.entries[i].valid && set.entries[i].tag == tag) {
            return i;  // Return the way where the hit occurred
        }
    }
    return -1;  // Miss
}
/**************************************************/
int find_evict_index(CacheSet& set){
  int lru_index = -1;
  int64_t oldest_time = std::numeric_limits<int64_t>::max();
  for (int i = 0; i < AHRT_ASSOC; ++i) {
    if (!set.entries[i].valid) {
        lru_index = i;
        break;
    } else if (set.entries[i].timestamp < oldest_time) {
        oldest_time = set.entries[i].timestamp;
        lru_index = i;
    }
  }
  return lru_index;
}
/**************************************************/
void update_cache(Addr address, long long history) {
    unsigned int set_index = compute_set_index(address);
    Addr tag = compute_tag(address);
    CacheSet& set = cache[set_index];
    
    int lru_index = check_hit(set, tag);
    
    if (lru_index == -1) { // No invalid entries, perform LRU replacement
        lru_index = find_evict_index(set);
    }

    set.entries[lru_index].tag = tag;
    set.entries[lru_index].history = history;
    set.entries[lru_index].valid = true;
    set.entries[lru_index].timestamp = get_current_time();
}
/**************************************************/
long long get_cache_entry(Addr address) {
    unsigned int set_index = compute_set_index(address);
    Addr tag = compute_tag(address);
    CacheSet& set = cache[set_index];
    
    int lru_index = check_hit(set, tag);
    
    if (lru_index == -1) { // No invalid entries, perform LRU replacement
        lru_index = find_evict_index(set);
        set.entries[lru_index].history = 0;
    }
    
    set.entries[lru_index].tag = tag;
    set.entries[lru_index].valid = true;
    set.entries[lru_index].timestamp = get_current_time();
    return set.entries[lru_index].history;
}
/**************************************************/

void bp_twolevel_init_hhrt(){
  currentTime = 0;
  
  int rows = pow(2, GHR_SIZE);
  pattern_history_table = (uns8*)malloc(rows * sizeof(uns8));

  for (int i = 0; i < rows; i++) {
    pattern_history_table[i] = 1; // not taken
  }

  hash_hrt = (long long*)malloc(HRT_SIZE * sizeof(long long));
  for (int j = 0; j < static_cast<int>(HRT_SIZE); j++) {
    hash_hrt[j] = 0;
  }
}
/**************************************************/
void bp_twolevel_init_ahrt(){
  currentTime = 0;
  
  int rows = pow(2, GHR_SIZE);
  pattern_history_table = (uns8*)malloc(rows * sizeof(uns8));

  for (int i = 0; i < rows; i++) {
    pattern_history_table[i] = 1; // not taken
  }
  
  int AHRT_SETS = HRT_SIZE/AHRT_ASSOC;
  cache.resize(AHRT_SETS);

  std::cout << "Initializing branch predictor:" << std::endl;
  std::cout << "HRT_SIZE: " << HRT_SIZE << std::endl;
  std::cout << "GHR_SIZE: " << GHR_SIZE << std::endl;
  std::cout << "AHRT_SETS: " << AHRT_SETS << std::endl;
  std::cout << "Cache Size: " << cache.size() << std::endl;
  std::cout << "Associativity: " << cache[0].entries.size() << std::endl;
}
/**************************************************/
uns8 bp_twolevel_pred_hhrt(Op* op) {
  const Addr address = op->oracle_info.pred_addr;
  unsigned int ghrIndex = hash(address);
	unsigned int ghr = hash_hrt[ghrIndex];

  const uns8 pred = (pattern_history_table[ghr] >= 2);
  return pred;
}
/**************************************************/
uns8 bp_twolevel_pred_ahrt(Op* op) {
  const Addr address = op->oracle_info.pred_addr;
  long long ghr = get_cache_entry(address);
  unsigned int index = static_cast<unsigned int> (ghr);

  const uns8 pred = (pattern_history_table[index] >= 2);
  return pred;
}
/**************************************************/
void bp_twolevel_update_hhrt(Op* op) {
  const Addr address = op->oracle_info.pred_addr;
  unsigned int ghrIndex = hash(address);
  unsigned int ghr = hash_hrt[ghrIndex];
  
  bool actual_outcome = (op->oracle_info.dir == TAKEN);

  if (actual_outcome) {
    if (pattern_history_table[ghr] < 3) {
      pattern_history_table[ghr] += 1;
    }
  } else {
    if (pattern_history_table[ghr] > 0) {
      pattern_history_table[ghr] -= 1;
    }
  }

  hash_hrt[ghrIndex] = ((ghr << 1) | actual_outcome) & ((1 << GHR_SIZE) - 1);
}
/**************************************************/
void bp_twolevel_update_ahrt(Op* op) {
  const Addr address = op->oracle_info.pred_addr;
  long long ghr = get_cache_entry(address);
  unsigned int index = static_cast<unsigned int> (ghr);

  bool actual_outcome = (op->oracle_info.dir == TAKEN);

  if (actual_outcome) {
    if (pattern_history_table[index] < 3) {
      pattern_history_table[index] += 1;
    }
  } else {
    if (pattern_history_table[index] > 0) {
      pattern_history_table[index] -= 1;
    }
  }
  ghr = ((ghr << 1) | actual_outcome) & ((1 << GHR_SIZE) - 1);
  update_cache(address, ghr);
}
/**************************************************/
}  //end of namespace

/**************************************************/

void bp_twolevel_timestamp(Op* op) {}
void bp_twolevel_recover(Recovery_Info* info) {}
void bp_twolevel_spec_update(Op* op) {}
void bp_twolevel_retire(Op* op) {}
uns8 bp_twolevel_full(uns proc_id) { return 0; }

/**************************************************/
void bp_twolevel_init() {
  
  if(HRT_TYPE == "HHRT"){
    nm::bp_twolevel_init_hhrt();
  }
  else{
    nm::bp_twolevel_init_ahrt();
  }
}
/**************************************************/
uns8 bp_twolevel_pred(Op* op) {
  if(HRT_TYPE == "HHRT"){
    return nm::bp_twolevel_pred_hhrt(op);
  }
  else {
    return nm::bp_twolevel_pred_ahrt(op);
  }
}
/**************************************************/
void bp_twolevel_update(Op* op) {
  if(HRT_TYPE == "HHRT"){
    nm::bp_twolevel_update_hhrt(op);
  }
  else{
    nm::bp_twolevel_update_ahrt(op);
  }
}
/**************************************************/