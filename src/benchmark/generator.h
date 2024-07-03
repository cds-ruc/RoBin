/*
 * The MIT License (MIT)
 *
 * Copyright (C) 2022-2023 Feng Ren, Tsinghua University 
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
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef _GENERATOR_H_
#define _GENERATOR_H_
#include <cmath>
#include <mutex>
#include <atomic>
#include <cstdlib>
#include <cstdint>
#include <cassert>
#include <random>

#include "new_zipf.h"

const uint64_t kFNVOffsetBasis64 = 0xCBF29CE484222325;
const uint64_t kFNVPrime64 = 1099511628211;

inline uint64_t FNVHash64(uint64_t val) {
    uint64_t hash = kFNVOffsetBasis64;

    for (int i = 0; i < 8; i++) {
        uint64_t octet = val & 0x00ff;
        val = val >> 8;

        hash = hash ^ octet;
        hash = hash * kFNVPrime64;
    }
    return hash;
}

class Generator {
public:
    virtual uint64_t next() = 0;

    virtual uint64_t last() = 0;

    virtual ~Generator() {}
};

class ZipfianGenerator {
public:
    static const uint64_t kMaxNumItems = (UINT64_MAX >> 24);

    ZipfianGenerator(uint64_t min, uint64_t max, double zipfian_const, uint64_t seed) :
            num_items_(max - min + 1), base_(min) {
        assert(num_items_ >= 2 && num_items_ < kMaxNumItems);
        mehcached_zipf_init(&state_, num_items_, zipfian_const, seed);
        next(num_items_);
    }

    ZipfianGenerator(uint64_t num_items, double zipfian_const, uint64_t seed) :
            ZipfianGenerator(0, num_items - 1, zipfian_const, seed) {}
    
    const zipf_gen_state& get_state() const {
        return state_;
    }

    uint64_t next(uint64_t num) {
        return last_value_ = base_ + mehcached_zipf_next(&state_) % num;
    }

    uint64_t next() { return next(num_items_); }

    uint64_t last() { return last_value_; }

private:
    struct zipf_gen_state state_;
    uint64_t num_items_;
    uint64_t base_;
    uint64_t last_value_;
};

// class ScrambledZipfianGenerator : public Generator {
// public:
//     ScrambledZipfianGenerator(uint64_t min, uint64_t max, double zipfian_const) :
//             base_(min), num_items_(max - min + 1),
//             generator_(min, max, zipfian_const) {}

//     ScrambledZipfianGenerator(uint64_t num_items, double zipfian_const) :
//             ScrambledZipfianGenerator(0, num_items - 1, zipfian_const) {}

//     uint64_t next() {
//         return scramble(generator_.next());
//     }

//     uint64_t last() {
//         return scramble(generator_.last());
//     }

// private:
//     const uint64_t base_;
//     const uint64_t num_items_;
//     ZipfianGenerator generator_;

//     uint64_t scramble(uint64_t value) const {
//         return base_ + FNVHash64(value) % num_items_;
//     }
// };



#endif //_GENERATOR_H