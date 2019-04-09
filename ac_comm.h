#pragma once

#include <stdint.h>

namespace ac {

    enum { CASE_IGNORE = 1 };

    enum { // 8 bits
        // for STATE & FAKE_STATE
        S_FAKE = 1,
        S_FINAL = 2,
        S_DOUBLE_FINAL = 4,
        S_FINAL_ON_FAIL = 8,
    };

    struct mem_bound_t {
        const uint8_t* data;
        const uint8_t* tail;
    };

    size_t mem_bound_size(mem_bound_t& mb) {
        return mb.tail - mb.data;
    }
    void mem_bound_init(mem_bound_t& mb, const char* str) {
        mb.data = (uint8_t*)str;
        mb.tail = mb.data + (str ? strlen(str) : 0);
    }
    void mem_bound_init(mem_bound_t & mb, const std::string & str) {
        mb.data = (uint8_t*)str.c_str();
        mb.tail = mb.data + str.size();
    }
    void mem_bound_init(mem_bound_t & mb, std::string && str) {
        mb.data = (uint8_t*)str.c_str();
        mb.tail = mb.data + str.size();
    }
    void mem_bound_init(mem_bound_t & mb, const void* ptr, size_t size) {
        mb.data = (uint8_t*)ptr;
        mb.tail = mb.data + size;
    }

    template < typename T>
    struct final_mask {
        static const size_t highest_bit = sizeof(T) * 8 - 1;
        static const size_t mask = 1 << highest_bit;
        static const size_t rest_mask = ((T)-1) >> 1;
    };
    struct BaseAcc {
        template <typename K>
        static inline void set_final(K& k)  {
            k |= final_mask<K>::mask;
        }
        template < typename K>
        static inline bool is_final(const K& k)  {
            return !!(k & final_mask<K>::mask);
        }
        template < typename K>
        static inline K get_value(const K& k)  {
            return k & final_mask<K>::rest_mask;
        }
    };
};