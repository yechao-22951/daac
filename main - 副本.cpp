#include <stdio.h>
#include "mp_sm_mini_ac_builder.h"
#include <random>
#include <deque>
#include <iostream>
#include <fstream>
#include "SuccinctBitVector.hpp"
#include <libcuckoo/cuckoohash_map.hh>

std::random_device generator;

using namespace sm::algo;
static inline uint8_t lower(uint8_t ch) {
    if ('A' >= ch && 'Z' >= 'ch')
        return ch + 'a' - 'A';
    return ch;
}
static inline uint8_t _byte(uint8_t ch, uint8_t opt) {
    return opt & ACFF_CASE_SENSITIVE ? ch : lower(ch);
}

#pragma pack(1)

class byte_coder_t {
protected:
    struct _stat_t {
        uint8_t ch = 0;
        size_t  count = 0;
        static bool less_by_count(const _stat_t& l, const _stat_t& r) {
            return l.count > r.count;
        }
    };
protected:
    uint8_t table_[0x100] = {};
    uint8_t max_code = 0;
public:
    byte_coder_t() {};
    uint8_t translate(uint8_t ch) const {
        return table_[ch];
    }
    uint8_t* table() {
        return table_;
    }
    uint8_t operator [] (uint8_t ch) const {
        return table_[ch];
    }
    size_t code_limit() {
        return max_code;
    }
    void clear() {
        memset(table_, 0, sizeof(table_));
    }
    size_t build(const keywords_t& kws, uint8_t options) {
        size_t total = 0;
        std::vector<_stat_t> stats(0x100);
        for (size_t i = 0; i < stats.size(); ++i) {
            stats[i].ch = i;
            stats[i].count = 0;
        }
        max_code = 0;
        for (auto& kw : kws) {
            total += kw.size();
            for (size_t i = 0; i < kw.size(); ++i) {
                uint8_t ch = _byte(kw[i], options);
                if (!stats[ch].count) max_code++;
                stats[ch].count++;
            }
        }
        std::sort(stats.begin(), stats.end(), _stat_t::less_by_count);
        size_t code = 0;
        for (size_t i = 0; i < stats.size(); ++i) {
            uint8_t ch = stats[i].ch;
            if (stats[i].count) {
                table_[ch] = code++;
            }
            else {
                table_[ch] = max_code;
            }
        }
        if (options & ACFF_CASE_SENSITIVE) {
        }
        else {
            for (uint8_t ch = 'A'; ch < 'Z'; ++ch) {
                table_[ch] = table_[ch + 'a' - 'A'];
            }
        }
        return total;
    }
};


template < typename st_idx_t = uint16_t >
class daac_t {
public:
    struct state_t {
        uint8_t attr = 0;           // B_FINAL
        st_idx_t base = 0;          // start from in array
        st_idx_t check = 0;         // parent state
        st_idx_t fail = 0;          // fail_link
        uint32_t user = 0;
    };
protected:
    struct level_info_t {
        std::vector<st_idx_t> state;
        level_info_t(size_t size) {
            state.resize(size, 0);
        }
        void reset() {
            state.resize(state.size(), 0);
        }
    };
public:

protected:
    using store_t = std::vector<state_t>;
    store_t states_;
    uint8_t options_ = 0;
    byte_coder_t coder_;
    state_t* array_ = 0;

protected:

    size_t state_try_move(size_t state, uint8_t ch) {
        state_t* from = da_node(state);
        if (!from->base) return 0;
        size_t next = from->base + ch;
        state_t* node = da_node(next);
        if (!node) return 0;
        return next;
    }
    state_t* da_node(size_t s, bool alloc = false) {
        const size_t n = states_.size();
        if (s >= n) {
            if (!alloc) return NULL;
            states_.reserve(states_.size() * 1.5);
            states_.resize(s + 1);
        }
        return &states_[s];
    }
    size_t da_current_size() {
        return states_.size();
    }

    long build_level(const keywords_t & keywords, size_t strpos, level_info_t * up_level, level_info_t * level) {
        size_t rest = 0;
        for (size_t i = 0; i < keywords.size(); ++i)
        {
            const keyword_t& kw = keywords[i];
            if (strpos > kw.size())
                continue;

            rest++;

            size_t parent_state = up_level->state[i];
            state_t* parent = da_node(parent_state);

            if (strpos == kw.size()) {
                if (parent->attr & B_FINAL) {
                    // 同 一个状态，绑定到多个 用户数据
                }
                else {
                    parent->attr |= B_FINAL;
                    // 同 一个状态，绑定到第一个 用户数据
                    parent->user = kw.user_data();
                }
                continue;
            }

            uint8_t ch = kw[strpos];
            //ch = _byte(ch, options_);
            ch = coder_[ch];

            if (!parent->base)
                parent->base = da_current_size();

            size_t new_state = parent->base + ch;
            if (new_state != (st_idx_t)new_state)
                return -1;

            level->state[i] = new_state;

            state_t * new_state_node = da_node(new_state);
            if (new_state_node) continue;

            new_state_node = da_node(new_state, true);
            // alloc here
            new_state_node->check = parent_state;
            // 设置失败节点
            parent = da_node(parent_state);
            size_t fail_state = parent->fail;
            while (fail_state != parent_state) {
                state_t* fail_node = da_node(fail_state);
                size_t target = state_try_move(fail_state, ch);
                if (target) {
                    new_state_node->fail = target;
                    //state_t * target_node = da_node(target);
                    //// 设置等效匹配节点
                    //for (; target; target = target->fail) {
                    //    if (da_node(target)->attr & B_FINAL) {
                    //        //new_state_node->match = new_state_node->fail;
                    //        new_state_node->attr |= B_HAS_MATCH;
                    //        break;
                    //    }
                    //}
                    break;
                }
                // 找下一个失败节点
                st_idx_t next_fail = fail_node->fail;
                if (next_fail == fail_state)
                    break;
                fail_state = next_fail;
            }
        }
        return rest;
    }
public:
    void clear() {
        states_.clear();
        coder_.clear();
        options_ = 0;
        if (array_) delete array_;
    }
    bool build(keywords_t & keywords, uint8_t options) {
        size_t total = coder_.build(keywords, options);

        states_.reserve(total * coder_.code_limit() * 0.5);

        options_ = options;
        typename keyword_t::less_compare_t less((options & ACFF_CASE_SENSITIVE) != 0);
        std::sort(keywords.begin(), keywords.end(), less);

        state_t * root = da_node(0, true);
        root->base = 1;

        level_info_t level1(keywords.size());
        level_info_t level2(keywords.size());

        level_info_t * lev_curr = &level1, *lev_next = &level2;
        long rest = keywords.size();
        for (size_t i = 0; rest; ++i) {
            rest = build_level(keywords, i, lev_curr, lev_next);
            if (rest < 0) return false;
            std::swap(lev_curr, lev_next);
            lev_next->reset();
        }
        size_t state_count = states_.size();
        size_t used_states = 0;
        array_ = (state_t*)calloc(state_count + 1, sizeof(state_t));
        for (size_t i = 0; i < states_.size(); ++i) {
            array_[i] = states_[i];
            if (array_[i].base || array_[i].attr) {
                used_states++;
            }
        }
        array_[state_count] = state_t{};
        //states_.clear();
        return true;
    }

public:
    // T::size, T::[] -> StateNode
    struct State {
        st_idx_t id;            // 
        const state_t* state;        // 
    };

    static const state_t InvalidState = {};

    class da_view_t {
    public:
        const state_t* states_;
        size_t size_;
    public:
        size_t size() { return size_; };
        State operator [] (size_t idx) {
            return states_[idx];
        }
    };

    da_view_t get_view() {
        const size_t size = states_.size();
        if (size) return da_view_t{ &states_[0], size };
        return da_view_t{ 0, 0 };
    }

    template < typename T>
    void make(T & searcher) {
        searcher.resize(states_.size());
        for (size_t i = 0; i < states_.size(); ++i) {
            if (states_[i].base || states_[i].attr) {
                searcher.add_state(i, states_[i]);
            }
        }
        searcher.done();
        return true;
    }

    class succinct_bitvec_t {
        using state_t = daac_t::state_t;
    public:
        succinct::dense::BitVector succinct_bitmap_;
        std::vector<state_t> succinct_states_;

        succinct_bitvec_t(size_t size) {
            succinct_bitmap_.init(size);
        }

        void add_state(size_t state_id, const state_t& state) {
            succinct_bitmap_.set_bit(state_id, 1);
            succinct_states_.push_back(state);
        }

        void done() {
            succinct_bitmap_.build_rank();
        }

        size_t size() {
            return succinct_bitmap_.length();
        }
        State operator [] (size_t state_id) {
            if (!state_id) return { 0, &succinct_states_[0] };
            return {
                state_id,
                succinct_bitmap_.lookup(state_id) ?
                    &succinct_states_[succinct_bitmap_.rank(state_id)] :
                    &InvalidState
            };
        }
    };

    class cuckoo_map_t {
        using state_hash_table_t = cuckoohash_map<st_idx_t, state_t>;
        state_hash_table_t states_;
        state_hash_table_t::locked_table table_;

        // for make
        void resize(size_t size) {
            states_.clear();
            states_.reserve(size / 1.5);
            table_ = state_hash_table_t::locked_table();
        }
        void add_state(size_t state_id, const state_t & state) {
            states_[state_id] = state;
        }
        void done() {
            table_ = states_.lock_table();
        }

        // for match
        size_t size() {
            return table_.size();
        }
        State operator [] (size_t state_id) {
            auto it = table_.find(state_id);
            return it == table_.end() ?
            {state_id, & InvalidState} :
            {state_id, & it->second};
        }
    };


    template <typename T>
    static int match(const T& states, uint8_t* code_table, size_t& state, mem_bound_t& buffer) {
        const uint8_t* data = buffer.data;
        const uint8_t* tail = buffer.tail;
        const size_t max_state = states.size();
        if (state >= max_state) state = 0;
        if (verbose) printf("\n\n B: ");
        State p = states[state];
        for (; data < tail; ++data) {
            uint8_t b8 = *data;
            uint8_t ch = code_table[b8];
            while (p.state != 0) {
                State next = states[p.node->base + ch];
                if (/*!next.node || */next.node->check != p.state) {
                    p = states[p.node->fail];
                    if (!p.state) break;
                    continue;
                }
                p = next;
                if (verbose) printf("%c|%d > ", b8, p.state);
                break;
            }
            if (p.state == 0) {
                State next = states[p.node->base + ch];
                if (/*!next.node || */next.node->check != p.state)
                    continue;
                p = next;
                if (verbose) printf("%c|%d > ", b8, p.state);
            }
            // 如果自己是FINAL节点，或者有FINAL节点链，则表示命中了
            if (p.node->attr & (B_FINAL | B_HAS_MATCH)) {
                state = p.state;
                // need skip the matched byte
                buffer.data = data + 1;
                return true;
            }
        }
        state = p.state;
        buffer.data = data;
        return false;
    }
};


using daac16_t = daac_t<uint16_t>;
using daac32_t = daac_t<uint32_t>;

#pragma pack()


#include <Windows.h>
int main() {

    std::ifstream file;
    file.open("prefixes.txt");
    std::deque<std::string> lines;
    while (!file.eof()) {
        std::string line;
        std::getline(file, line);
        if (line.empty())
            continue;
        lines.emplace_back(line);
    };
    file.close();

    lines.push_back("ka");

    keywords_t kws;
    kws.reserve(lines.size());
    for (auto& l : lines) {
        kws.push_back(l);
    }

    daac32_t daac;
    std::cout << GetTickCount() << std::endl;
    daac.build(kws, 0);
    std::cout << GetTickCount() << std::endl;

    auto daac_sm = daac.make_searcher();


    //mkSearchBuilder<uint16_t> mks_builder;
    //mks_builder.build(kws, 0);
    //sm::algo::mem_bound_t view = mks_builder.get_content();
    //printf("%p\n", mem_bound_size(view));

    //mkSearchBuilder<uint16_t>::Searcher* searcher = mks_builder.make_searcher();

    std::string text;
    text.resize(1024 * 1024 * 10);

    for (auto& ch : text) {
        ch = 'a' + (generator() % 26);
    }
    if (1) {
        for (size_t k = 0; k < 10; ++k) {
            for (uint8_t ch : text) {
                size_t s = 0, s1 = 0;
                bool m1 = daac_sm.match(s, ch);
                bool m2 = daac.match(s1, ch);
                if (s1 != s || m1 != m2) {
                    DebugBreak();
                }
            }
        }
    }
    if (1) {
        std::cout << GetTickCount() << std::endl;
        size_t s = 0, s1 = 0;
        mem_bound_t in, in2;
        size_t matched = 0, matched2 = 0;
        for (size_t k = 0; k < 1; ++k) {
            mem_bound_init(in, text);
            mem_bound_init(in2, text);
            while (mem_bound_size(in) || mem_bound_size(in2)) {
                size_t prev_s = s, prev_s1 = s1;
                mem_bound_t prev_in = in, prev_in2 = in2;
                matched2 += daac.match(s1, in2) ? 1 : 0;
                matched += daac_sm.match(s, in) ? 1 : 0;
                if (s != s1 || matched != matched2 || in.data != in2.data) {
                    s = prev_s; s1 = prev_s1;
                    in = prev_in; in2 = prev_in2;
                    daac.verbose = true;
                    daac_sm.verbose = true;
                    DebugBreak();
                    int a = 1;
                }
            }
        }
        std::cout << GetTickCount() << std::endl;
        std::cout << matched << std::endl;
        std::cout << matched2 << std::endl;
    }
    if (1) {
        const size_t loop = 50;
        {
            std::cout << "SparseVector Version" << std::endl;
            size_t s = 0;
            mem_bound_t in;
            size_t matched = 0;
            std::cout << GetTickCount() << std::endl;
            for (size_t k = 0; k < loop; ++k) {
                mem_bound_init(in, text);
                while (mem_bound_size(in)) {
                    matched += daac.match(s, in) ? 1 : 0;
                }
            }
            std::cout << GetTickCount() << std::endl;
            std::cout << matched << std::endl;
        }
        {
            std::cout << "Succicnt Version" << std::endl;
            size_t s = 0;
            mem_bound_t in;
            size_t matched = 0;
            std::cout << GetTickCount() << std::endl;
            for (size_t k = 0; k < loop; ++k) {
                mem_bound_init(in, text);
                while (mem_bound_size(in)) {
                    matched += daac_sm.match(s, in) ? 1 : 0;
                }
            }
            std::cout << GetTickCount() << std::endl;
            std::cout << matched << std::endl;
        }
    }

}

//template < typename T >
//struct byte_entry_t {
//    uint8_t code;
//    T value;
//};
//
//class dense_byte_map_t {
//protected:
//    std::vector<uint8_t> content_;
//public:
//    void build( const byte_entry_t * items, size_t size ) {
//
//    }
//};
//
//
//
