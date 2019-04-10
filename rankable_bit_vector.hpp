/*-----------------------------------------------------------------------------
 *  SuccinctBitVector.hpp - A x86/64 optimized rank/select dictionary
 *
 *  Coding-Style: google-styleguide
 *      https://code.google.com/p/google-styleguide/
 *
 *  Copyright 2012 Takeshi Yamamuro <linguin.m.s_at_gmail.com>
 *-----------------------------------------------------------------------------
 */

#ifndef __SUCCINCTBITVECTOR_HPP__
#define __SUCCINCTBITVECTOR_HPP__

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <climits>
#include <vector>
#include <memory>

#include <nmmintrin.h>

#ifndef NDEBUG
#define __assert(x) 
#else
#define __assert(x)
#endif 

namespace succinct {
    namespace dense {

        /* namespace { */

        /* Defined by BSIZE */
        typedef uint64_t  block_t;

        static const size_t BSIZE = 64;
        static const size_t PRESUM_SZ = 128;
        static const size_t CACHELINE_SZ = 64;

#ifdef __USE_SSE_POPCNT__
        static uint64_t popcount64(block_t b) {
#if defined(__x86_64__) || defined(_WIN64)
            return _mm_popcnt_u64(b);
#else
            return _mm_popcnt_u32((b >> 32) & 0xffffffff) +
                _mm_popcnt_u32(b & 0xffffffff);
#endif
        }
#else
        static const uint8_t popcountArray[] = {
          0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
          1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
          1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
          2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
          1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
          2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
          2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
          3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8
        };

        static uint64_t popcount64(block_t b) {
            return	popcountArray[(b >> 56) & 0xff] + popcountArray[(b >> 48) & 0xff] +
                popcountArray[(b >> 40) & 0xff] + popcountArray[(b >> 32) & 0xff] +
                popcountArray[(b >> 24) & 0xff] + popcountArray[(b >> 16) & 0xff] +
                popcountArray[(b >> 8) & 0xff] + popcountArray[b & 0xff];
        }
#endif /* __USE_SSE_POPCNT__ */

        class BitVector {
        public:
            BitVector() : size_(0) {}
            ~BitVector() throw() {}

            void init(uint64_t len) {
                __assert(len != 0);

                size_ = len;
                size_t bnum = (size_ + BSIZE - 1) / BSIZE;

                B_.resize(bnum);
                for (size_t i = 0; i < bnum; i++)
                    B_[i] = 0;
            }

            void set_bit(uint64_t pos, uint8_t bit) {
                __assert(pos < size_);
                B_[pos / BSIZE] |= uint64_t(1) << (pos % BSIZE);
            }

            inline bool lookup(uint64_t pos) const {
                __assert(pos < size_);
                return (B_[pos / BSIZE] & (uint64_t(1) << (pos % BSIZE))) > 0;
            }

            const block_t get_block(uint64_t pos) const {
                __assert(pos < size_);
                return B_[pos];
            }

            uint64_t length() const {
                return size_;
            }

            uint64_t bsize() const {
                return B_.size();
            }

            void build_rank() {
                size_t count = B_.size();
                rank_.resize(count);
                uint64_t rank = 0;
                for (size_t i = 0; i < count; i++) {
                    rank_[i] = rank;
                    rank += popcount64(B_[i]);
                }
            }

            // 不是标准的rank1, 是 [0,pos) 之间有几个1,而不是 [0,pos]
            __forceinline size_t rank(size_t pos) const {
                __assert(pos <= size_);
                if (!pos) return 0;
                const size_t bi = pos / BSIZE;
                uint32_t rank = rank_[bi];
                const uint64_t b1 = B_[bi];
                const size_t r = pos & (BSIZE - 1);
                const uint64_t mask = (uint64_t(1) << r) - 1;
                rank += popcount64(b1 & mask);
                return rank;
            }

            size_t room() const {
                return B_.size() * sizeof(block_t) + rank_.size() * sizeof(uint32_t);
            }

        private:
            size_t size_;
            std::vector<block_t>  B_;
            std::vector<uint32_t> rank_;
        }; /* BitVector */

    } /* dense */
} /* succinct */

#endif /* __SUCCINCTBITVECTOR_HPP__ */
