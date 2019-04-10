#pragma once

#include <stdint.h>
#include <vector>
#include <unordered_map>
#include "ac_comm.h"
#include "ac_byte_coder.h"
#include "ac_keywords.h"

namespace ac {

    using StateId = uint32_t;
    enum { FROM_INVALID = -1 };

    struct State {
        StateId     base = 0;               // start from in array
        StateId     check = FROM_INVALID;   // parent state
        StateId     fail = 0;               // fail_link
        uint8_t     attr = 0;               // S_FINAL
    };

    struct MatchPair {
        StateId     state = 0;
        StateId     kwidx = 0;
        MatchPair() {}
        MatchPair(const std::pair<StateId, StateId>& pair) {
            state = pair.first;
            kwidx = pair.second;
        }
    };

    class DoubleArrayBuilder {
    public:
        using Keyword = Keyword;
        using Keywords = std::vector<Keyword>;
    protected:
        struct level_info_t {
            std::vector<StateId> state;
            level_info_t(size_t size) {
                state.resize(size, 0);
            }
            void reset() {
                state.resize(state.size(), 0);
            }
        };
        using state_vector_t = std::vector<State>;
        using state_to_kwidx_t = std::unordered_multimap<StateId, StateId>;
    protected:
        bool verbose = false;
        uint8_t options_ = 0;
        size_t valid_states_ = 0;
        state_vector_t states_;
        byte_coder_t coder_;
        state_to_kwidx_t match_set_;
    protected:
        const uint8_t* __code_table() {
            return coder_.table();
        }
        bool __try_get_next_state(size_t& state, uint8_t code) {
            size_t next = states_[state].base + code;
            State* next_node = __state_node(next);
            if (!next_node) return false;    // back to root
            if (next_node->check != state)
                return false;   // back to root
            state = next;
            return true;
        }
        State* __state_node(size_t s, bool alloc = false) {
            const size_t n = states_.size();
            if (s >= n) {
                if (!alloc) return NULL;
                states_.reserve(states_.size());
                states_.resize(s + 1);
            }
            return &states_[s];
        }
        size_t __states_size() {
            return states_.size();
        }
    protected:
        long __build_level(const Keywords & keywords, size_t strpos, level_info_t * prev_lev, level_info_t * this_lev) {
            size_t rest = 0;
            for (size_t i = 0; i < keywords.size(); ++i)
            {
                const Keyword& kw = keywords[i];
                if (strpos > kw.size())
                    continue;

                rest++;

                StateId parent_state = prev_lev->state[i];
                State* parent = __state_node(parent_state);

                if (strpos == kw.size()) {
                    auto it = match_set_.find(parent_state);
                    match_set_.emplace_hint(it, parent_state, kw.index());
                    parent->attr |= S_FINAL;
                    continue;
                }

                uint8_t b8 = kw[strpos];
                uint8_t ch = __code_table()[b8];

                if (!parent->base)
                    parent->base = __states_size();

                size_t new_state_ = parent->base + ch;
                if (new_state_ != (StateId)new_state_)
                    return -1;

                StateId new_state = this_lev->state[i] = new_state_;

                State * new_state_node = __state_node(new_state);
                if (new_state_node && new_state_node->check != FROM_INVALID)
                    continue;

                //putchar(b8);

                new_state_node = __state_node(new_state, true);
                new_state_node->check = parent_state;
                new_state_node->fail = 0;
                // 设置失败节点
                size_t fail_state = parent->fail;
                while (fail_state != parent_state) {
                    size_t target = fail_state;
                    if (__try_get_next_state(target, ch)) {
                        new_state_node->fail = target;
                        break;
                    }
                    // 找下一个失败节点
                    State* fail_node = __state_node(fail_state);
                    StateId next_fail = fail_node->fail;
                    if (next_fail == fail_state)
                        break;
                    fail_state = next_fail;
                }
            }
            return rest;
        }
    public:
        void clear() {
            options_ = 0;
            states_.clear();
            coder_.clear();
            valid_states_ = 0;
        }
        const uint8_t* code_table() {
            return coder_.table();
        }
        bool build(Keywords & keywords, uint8_t options) {

            clear();

            fill_index(keywords);

            size_t total = coder_.build(keywords, options);

            states_.reserve(total * coder_.max_code() * 0.5);

            options_ = options;

            bool ignore_case = !!(options & CASE_IGNORE);

            typename Keyword::less_compare_t less(ignore_case);
            std::sort(keywords.begin(), keywords.end(), less);

            State* root = __state_node(0, true);
            root->base = __states_size();

            level_info_t level1(keywords.size());
            level_info_t level2(keywords.size());

            level_info_t* lev_curr = &level1, * lev_next = &level2;
            long rest = keywords.size();
            for (size_t i = 0; rest; ++i) {
                rest = __build_level(keywords, i, lev_curr, lev_next);
                if (rest < 0) return false;
                std::swap(lev_curr, lev_next);
                lev_next->reset();
            }

            valid_states_ = 0;
            for (size_t i = 0; i < states_.size(); ++i) {
                if (states_[i].base || states_[i].attr) {
                    valid_states_++;
                }
            }
            return true;
        }

        template < typename T>
        bool make(T & searcher) {
            searcher.set_code_table(code_table());
            bool f = searcher.resize(states_.size(), valid_states_, match_set_.size());
            if (!f) return false;
            for (StateId i = 0; i < states_.size(); ++i) {
                auto state = states_[i];
                if (state.base || state.attr) {
                    if (state.attr & S_FINAL) {
                        auto range = match_set_.equal_range(i);
                        std::vector<MatchPair> matches(range.first, range.second);
                        f = searcher.add_state(i, state, matches.size(), matches.data());
                    }
                    else {
                        f = searcher.add_state(i, state, 0, 0);
                    }
                    if (!f)
                        return false;
                }
            }
            return searcher.done();
        }
    };
};