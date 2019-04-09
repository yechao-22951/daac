#pragma once

#include <vector>
#include <string_view>
#include <stdint.h>
#include <vector>
#include <deque>
#include <algorithm>
#include <string>
#include <string_view>
#include <bitset>

namespace sm {
    namespace algo {

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
        enum {
            ACFF_CASE_SENSITIVE = 1,
        };
#pragma pack(1)
        struct skSearch {
            struct state_t {
                uint8_t accpet;
                uint8_t fail;       // ?
            };
            uint8_t flags;
            uint8_t scount;
            state_t states[0];

            // true, matched
            // false, unmatched
            bool match(size_t& state, uint8_t ch) {
                if (state >= scount) state = 0;
                for (;;) {
                    if (states[state].accpet == ch) {
                        state = state + 1;
                        if (state == scount)
                            return true;
                        break;
                    }
                    if (state) {
                        state = states[state - 1].fail;
                        continue;
                    }
                    if (!state) break;
                }
                return false;
            }
            bool match(size_t & state, mem_bound_t & buffer) {
                if (state >= scount) state = 0;
                const uint8_t * data = buffer.data;
                const uint8_t * tail = buffer.tail;
                for (; data < tail; ++data) {
                    const uint8_t ch = *data;
                    for (;;) {
                        if (states[state].accpet == ch) {
                            state = state + 1;
                            if (state == scount) {
                                buffer.data = data;
                                return true;
                            }
                            break;
                        }
                        if (state) {
                            state = states[state - 1].fail;
                            continue;
                        }
                        if (!state)
                            break;
                    }
                }
                buffer.data = data;
                return false;
            }

            // true, continue;
            // false, breaked;
            template <typename callback_t >
            void execute(size_t & state, mem_bound_t & buffer, callback_t && on_hit) {
                if (state >= scount) state = 0;
                const uint8_t * data = buffer.data;
                const uint8_t * tail = buffer.tail;
                for (; data < tail; ++data) {
                    const uint8_t ch = *data;
                    for (;;) {
                        if (states[state].accpet == ch) {
                            state = state + 1;
                            if (state == scount) {
                                if (!on_hit(data)) {
                                    buffer.data = data;
                                    return;
                                }
                            }
                            break;
                        }
                        if (state) {
                            state = states[state - 1].fail;
                            continue;
                        }
                        if (!state)
                            break;
                    }
                }
                buffer.data = data;
                return;
            }
        };

        enum {
            B_FINAL = 1,
            B_HAS_MATCH = 2,
            B_BOOSTED = 4,
        };

        template < typename st_idx_t >
        struct mkSearch {
            struct state_t {
                uint8_t     code;
                uint8_t     attr;
                uint8_t     len;
                st_idx_t    base;
                st_idx_t    fail;
                // mkSearch 不需要
                //st_idx_t  match;
            };
            struct state_b_t {
                uint8_t     attr;
                uint8_t     len;
                st_idx_t    base;
                st_idx_t    fail;
            };
            st_idx_t scount;
            uint16_t flags;
            state_t states[0];

            static inline bool less_code(const state_t& l, const state_t& r) {
                return l.code < r.code;
            }

#define CASE_STATE_MOVE_CHECK(x)    case x: if (first[x-1].code == ch) return first + x - 1;
#define CASE_STATE_MOVE_NONE()      case 0: return NULL;

            static __forceinline state_t * move_state(state_t * root, state_t * state, uint8_t ch) {
                state_t* first = state->base + root;
                switch (state->len) {
                    CASE_STATE_MOVE_CHECK(8);
                    CASE_STATE_MOVE_CHECK(7);
                    CASE_STATE_MOVE_CHECK(6);
                    CASE_STATE_MOVE_CHECK(5);
                    CASE_STATE_MOVE_CHECK(4);
                    CASE_STATE_MOVE_CHECK(3);
                    CASE_STATE_MOVE_CHECK(2);
                    CASE_STATE_MOVE_CHECK(1);
                    CASE_STATE_MOVE_NONE();
                }
                // lower_bound is slow ...
                state_t* last = first + state->len;
                state_t fake; fake.code = ch;
                first = std::lower_bound(first, last, fake, less_code);
                if (first == last) return NULL;
                if (first->code == ch)
                    return first;
                return NULL;
            }
            inline state_t* state_move(state_t * state, uint8_t ch) {
                return move_state(states, state, ch);
            }
            state_t* root() {
                return states;
            }

            bool match(size_t & state, uint8_t ch) {
                ch = flags & ACFF_CASE_SENSITIVE ? ch : ::tolower(ch);
                state_t* root_ = root();
                //state_t* last = root_ + scount;
                if (state >= scount) state = 0;
                state_t * p = root_ + state;
                state_t * next = 0;
                while (p != root_) {
                    next = state_move(p, ch);
                    if (!next) {
                        p = p->fail + root_;
                        continue;
                    }
                    p = next;
                    break;
                }
                if (p == root_) {
                    p = state_move(p, ch);
                    if (!p) {
                        state = 0;
                        return false;
                    }
                }
                // 如果自己是FINAL节点，或者有FINAL节点链，则表示命中了
                if (p->attr & (B_FINAL | B_HAS_MATCH)) {
                    state = p - root_;
                    return true;
                }
                state = p - root_;
                return false;
            }

            bool match(size_t & state, mem_bound_t & buffer) {
                typedef mkSearch::state_t state_t;
                const uint8_t* data = buffer.data;
                const uint8_t* tail = buffer.tail;
                state_t* const root_ = root();
                //state_t* last = root_ + scount;
                if (state >= scount) state = 0;
                state_t * p = root_ + state;
                state_t * next = 0;
                for (; data < tail; ++data) {
                    auto ch = flags & ACFF_CASE_SENSITIVE ? *data : ::tolower(*data);
                    while (p != root_) {
                        next = state_move(p, ch);
                        if (!next) {
                            p = p->fail + root_;
                            continue;
                        }
                        p = next;
                        break;
                    }
                    if (p == root_) {
                        next = state_move(p, ch);
                        if (!next)
                            continue;
                        p = next;
                    }
                    // 如果自己是FINAL节点，或者有FINAL节点链，则表示命中了
                    if (p->attr & (B_FINAL | B_HAS_MATCH)) {
                        state = p - root_;
                        // need skip the matched byte
                        buffer.data = data + 1;
                        return true;
                    }
                }
                state = p - root_;
                buffer.data = data;
                return false;
            }
        };
#pragma pack()

    };
};