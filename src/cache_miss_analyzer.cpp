#include <unordered_map>
#include "cache_miss_analyzer.h"
#include <iostream>

std::unordered_map<uint64_t, bool> address_seen;

enum class MissType {
    COMPULSORY,
    CAPACITY,
    CONFLICT,
    OTHER
};

MissType categorize_miss(uint64_t address, int set_full) {
    
    if (address_seen.find(address) == address_seen.end()) { // address not found
        address_seen[address] = true;
        return MissType::COMPULSORY;
    } else if (set_full == 1) { // set is full
        return MissType::CAPACITY;
    } else {
        return MissType::CONFLICT;
    }
}

extern "C" {
    int log_cache_miss(Addr address, int set_full) {
        MissType miss_type = categorize_miss(address, set_full);
        return static_cast<int>(miss_type);
    }
}
