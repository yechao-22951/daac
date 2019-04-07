#include <stdio.h>
#include "mp_sm_mini_ac_builder.h"
#include <random>
#include <deque>
#include <iostream>
#include <fstream>
#include "rankable_bit_vector.hpp"
#include <libcuckoo/cuckoohash_map.hh>
#include <unordered_map>

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


template < typename st_idx_t = uint32_t >
class daac_t {
public:
    using id_t = st_idx_t;
    struct state_t {
        uint8_t attr = 0;           // B_FINAL
        st_idx_t base = 0;          // start from in array
        st_idx_t check = 0;         // parent state
        st_idx_t fail = 0;          // fail_link
        //uint32_t user = 0;
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
            states_.reserve(states_.size());
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

            id_t parent_state = up_level->state[i];
            state_t* parent = da_node(parent_state);

            if (strpos == kw.size()) {
                if (parent->attr & B_FINAL) {
                    // 同 一个状态，绑定到多个 用户数据
                }
                else {
                    parent->attr |= B_FINAL;
                    // 同 一个状态，绑定到第一个 用户数据
                    //parent->user = kw.user_data();
                }
                continue;
            }

            uint8_t ch = kw[strpos];
            //ch = _byte(ch, options_);
            ch = coder_[ch];

            if (!parent->base)
                parent->base = da_current_size();

            size_t new_state_ = parent->base + ch;
            if (new_state_ != (id_t)new_state_)
                return -1;
            id_t new_state = new_state_;

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
    uint8_t* code_table() {
        return coder_.table();
    }
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

#define DA_BASE(x) array_[x].base
#define DA_CHECK(x) array_[x].check
#define DA_FAIL(x) array_[x].fail

    bool match(size_t & p, uint8_t ch) {
        const size_t max_state = states_.size();
        if (p >= max_state) p = 0;
        //printf("\nb %c : %d -> ", ch, p);
        ch = coder_[ch];
        while (p != 0) {
            size_t next = DA_BASE(p) + ch;
            if (DA_CHECK(next) != p) {
                p = DA_FAIL(p);
                if (!p) break;
                continue;
            }
            p = next;
            //printf("%d -> ", p);
            break;
        }
        if (p == 0) {
            size_t next = DA_BASE(p) + ch;
            if (DA_CHECK(next) != p) {
                return false;
            }
            p = next;
            //printf("%d -> ", p);
        }
        // 如果自己是FINAL节点，或者有FINAL节点链，则表示命中了
        if (array_[p].attr & (B_FINAL | B_HAS_MATCH)) {
            return true;
        }
        return false;
    }

    bool verbose = false;
    int match(size_t & state, mem_bound_t & buffer) {
        const uint8_t* data = buffer.data;
        const uint8_t* tail = buffer.tail;
        const size_t max_state = states_.size();
        if (state >= max_state) state = 0;
        if (verbose) printf("\n\n A: ");
        size_t p = state;
        for (; data < tail; ++data) {
            uint8_t b8 = *data;
            uint8_t ch = coder_[b8];
            while (p != 0) {
                size_t next = DA_BASE(p) + ch;
                if (DA_CHECK(next) != p) {
                    p = DA_FAIL(p);
                    if (!p) break;
                    continue;
                }
                p = next;
                if (verbose) printf("%c|%d > ", b8, p);
                break;
            }
            if (p == 0) {
                size_t next = DA_BASE(p) + ch;
                if (DA_CHECK(next) != p)
                    continue;
                p = next;
                if (verbose) printf("%c|%d > ", b8, p);
            }
            // 如果自己是FINAL节点，或者有FINAL节点链，则表示命中了
            if (array_[p].attr & (B_FINAL | B_HAS_MATCH)) {
                state = p;
                // need skip the matched byte
                buffer.data = data + 1;
                return true;
            }
        }
        state = p;
        buffer.data = data;
        return false;
    }

public:

    struct State {
        st_idx_t id;            // 
        const state_t* state;        // 
    };

    static inline const state_t InvalidState = {};

    class double_array_t {
    public:
        size_t size_;
        const state_t* states_;
    public:
        using state_t = daac_t::state_t;
        using id_t = daac_t::id_t;
        using State = daac_t::State;
        inline size_t size() const {
            return size_;
        };
        __forceinline bool move_to(State& from, uint8_t code) const {
            id_t to = from.state->base + code;
            const state_t* target = states_ + to;
            bool success = (target->check == from.id);
            bool from_root = from.id > 0;
            if (!success && from_root) {
                from.id = 0;
                from.state = 0;
            }
            else {
                from.id = success ? to : from.state->fail;
                from.state = success ? target : (states_ + from.state->fail);
            }
            return success;
        }
        __forceinline State set_to(id_t id) const {
            return State{ id, states_ + id };
        }
        size_t room() const {
            return size_ * sizeof(state_t);
        }
    };

    double_array_t get_view() {
        const size_t size = states_.size();
        if (size) return double_array_t{ size,  &states_[0] };
        return double_array_t{ 0, 0 };
    }

    template < typename T>
    bool make(T& searcher) {
        size_t valid_count = 0;
        for (id_t i = 0; i < states_.size(); ++i) {
            if (states_[i].base || states_[i].attr) {
                valid_count++;
            }
        }
        searcher.resize(states_.size(), valid_count);
        for (id_t i = 0; i < states_.size(); ++i) {
            if (states_[i].base || states_[i].attr) {
                searcher.add_state(i, states_[i]);
            }
        }
        searcher.done();
        return true;
    }

    class succinct_bitvec_t {
    public:
        succinct::dense::BitVector succinct_bitmap_;
        std::vector<state_t> succinct_states_;
    public:
        using state_t = daac_t::state_t;
        using id_t = daac_t::id_t;
        using State = daac_t::State;

        void resize(size_t state_count, size_t valid_count) {
            succinct_bitmap_.init(state_count);
            succinct_states_.reserve(valid_count);
        }
        void add_state(id_t state_id, const state_t& state) {
            succinct_bitmap_.set_bit(state_id, 1);
            succinct_states_.push_back(state);
        }
        void done() {
            succinct_bitmap_.build_rank();
        }
        inline size_t size() const {
            return succinct_bitmap_.length();
        }

        __forceinline bool move_to(State& from, uint8_t code) const {
            bool from_root = from.id > 0;
            id_t to = from.state->base + code;
            bool success = succinct_bitmap_.lookup(to);
            const state_t* target = nullptr;
            if (success) {
                target = &succinct_states_[succinct_bitmap_.rank(to)];
                success = (target->check == from.id);
            }
            if (!success && from_root) {
                from.state = 0;
                return false;
            }
            if (!success) {
                to = from.state->fail;
                target = &succinct_states_[succinct_bitmap_.rank(to)];
            }
            from.id = to;
            from.state = target;
            return success;
        }
        __forceinline State set_to(id_t to) const {
            if (!to) return { 0, &succinct_states_[0] };
            return  { to, &succinct_states_[succinct_bitmap_.rank(to)] };
        }
        size_t room() const {
            return succinct_bitmap_.room() +
                succinct_states_.size() * sizeof(state_t);
        }
    };

    class cuckoo_map_t {
        using state_hash_table_t = cuckoohash_map<st_idx_t, state_t>;
        state_hash_table_t states_;
    public:
        using state_t = daac_t::state_t;
        using id_t = daac_t::id_t;
        using State = daac_t::State;
        // for make
        void resize(size_t state_count, size_t valid_count) {
            states_.clear();
            states_.reserve(valid_count);
        }
        void add_state(id_t state_id, const state_t& state) {
            states_.insert(state_id, state);
        }
        void done() {
        }
        // for match
        inline size_t size() const {
            return states_.size();
        }
        __forceinline State move_to(const State& from, uint8_t code) const {
            id_t to = from.state->base + code;
            const state_t* state = states_.find_mapped(to);
            return State{ to, state ? state : &InvalidState };
        }
        __forceinline State set_to(id_t to) const {
            const state_t* state = states_.find_mapped(to);
            return State{ to, state ? state : &InvalidState };
        }

        // 
        size_t room() const {
            return 0;
        }
    };

    class unordered_map_t {
        using state_hash_table_t = std::unordered_map<st_idx_t, state_t>;
        state_hash_table_t states_;
    public:
        using state_t = daac_t::state_t;
        using id_t = daac_t::id_t;
        using State = daac_t::State;
        // for make
        void resize(size_t state_count, size_t valid_count) {
            states_.clear();
            states_.reserve(valid_count);
        }
        void add_state(id_t state_id, const state_t& state) {
            states_[state_id] = state;
        }
        void done() {
        }
        // for match
        inline size_t size() const {
            return states_.size();
        }

        __forceinline bool move_to(State& from, uint8_t code) const {
            bool from_root = from.id > 0;
            id_t to = from.state->base + code;
            auto it = states_.find(to);
            bool success = it != states_.end();
            const state_t* state = 0;
            if( success ) {
                state = &it->second;
                success = state->check == from.id;
            }
            if(!success && from_root ) {
                from.state = 0;
                return false;
            }
            if( !success ) {
                to = from.state->fail;
                state = &(states_.find(to)->second);
            }

            from.id = to;
            from.state = state;

            return success;

        }

        __forceinline State set_to(id_t to) const {
            auto it = states_.find(to);
            const state_t* state = &it->second;
            return State{ to, state };
        }

        // 
        size_t room() const {
            return 0;
        }
    };

    class sorted_states_t {
        struct _item_t {
            st_idx_t id;
            state_t state;
            bool operator < (const _item_t& r) const {
                return id < r.id;
            }
        };
        using state_table_t = std::vector<_item_t>;
        state_table_t states_;
    public:
        using state_t = daac_t::state_t;
        using id_t = daac_t::id_t;
        using State = daac_t::State;
        // for make
        void resize(size_t state_count, size_t valid_count) {
            states_.clear();
            states_.reserve(valid_count);
        }
        void add_state(id_t state_id, const state_t& state) {
            states_.emplace_back(_item_t{ state_id, state });
        }
        void done() {
            std::sort(states_.begin(), states_.end());
        }
        // for match
        inline size_t size() const {
            return states_.size();
        }
        //__forceinline State move_to(const State& from, uint8_t code) const {
        //    id_t to = from.state->base + code;
        //    return { to, find_state(to) };
        //}
        __forceinline bool move_to(State& from, uint8_t code) const {
            bool from_root = from.id > 0;
            id_t to = from.state->base + code;
            const state_t* state = find_state(to);
            if (state) {
                state = state->check == from.id ? state : nullptr;
            }
            if (!state && from_root) {
                from.state = 0;
                return false;
            }
            if (!state) {
                to = from.state->fail;
                state = find_state(to);
            }
            from.id = to;
            from.state = state;
            return state != 0;
        }


        __forceinline State set_to(id_t to) const {
            return { to, find_state(to) };
        }
        size_t room() const {
            return sizeof(_item_t)* states_.size();
        }
        __forceinline const state_t* find_state(id_t state_id) const {
            if (!state_id) return &states_[0].state;
            _item_t key = { state_id };
            auto it = std::lower_bound(states_.begin(), states_.end(), key);
            bool miss = (it == states_.end()) || (it->id != state_id);
            return miss ? nullptr : &(it->state);
        }
    };
};

template <typename T>
static int ac_match(
    const T & machine,
    const uint8_t * code_table,
    typename T::id_t & state,
    mem_bound_t & buffer,
    bool verbose)
{
    const uint8_t* data = buffer.data;
    const uint8_t* tail = buffer.tail;
    const size_t max_state = machine.size();
    if (state >= max_state) state = 0;
    if (verbose) printf("\n\n B: ");
    typename T::State p = machine.set_to(state);
    for (; data < tail; ++data) {
        uint8_t b8 = *data;
        uint8_t ch = code_table[b8];
        for (; p.state; ) {
            if (!machine.move_to(p, ch))
                continue;
            // success
            if (p.state->attr & (B_FINAL | B_HAS_MATCH)) {
                state = p.id;
                // need skip the matched byte
                buffer.data = data + 1;
                return true;
            }
            break;
        }
        if (!p.state) {
            p = machine.set_to(0);
        }
    }
    state = p.id;
    buffer.data = data;
    return false;
}


using daac16_t = daac_t<uint16_t>;
using daac32_t = daac_t<uint32_t>;

#pragma pack()


#include <Windows.h>

template <typename T >
void speed_test(const T & searcher, const uint8_t * coder, const std::string & text, size_t loop) {
    const char* class_name = typeid(T).name();
    typename T::id_t state = 0;
    size_t total = text.size() * loop;
    mem_bound_t in;
    size_t matched = 0;
    size_t start = GetTickCount();
    for (size_t k = 0; k < loop; ++k) {
        mem_bound_init(in, text);
        while (mem_bound_size(in)) {
            matched += ac_match(searcher, coder, state, in, false) ? 1 : 0;
        }
    }
    double durtion = GetTickCount() - start;
    printf("%s , %zd bytes : %zd ms, %zd bytes, speed = %0.2f MB/s, matched = %zd\n",
        class_name, searcher.room(), (size_t)durtion, total, total / durtion * 1000 / 1024 / 1024, matched);
}

int main() {

    std::ifstream file;
    file.open("Untitled-1.txt");
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

    daac32_t::cuckoo_map_t cuckoo;
    daac32_t::succinct_bitvec_t bitvec;
    daac32_t::double_array_t normal = daac.get_view();
    daac32_t::unordered_map_t stlmap;
    daac32_t::sorted_states_t sorted;

    daac.make(cuckoo);
    daac.make(bitvec);
    daac.make(stlmap);
    daac.make(sorted);

    std::string text;
    text.resize(1024 * 1024 * 10);

    for (auto& ch : text) {
        ch = 'a' + (generator() % 26);
    }

    speed_test(normal, daac.code_table(), text, 10);
    speed_test(bitvec, daac.code_table(), text, 10);
    speed_test(sorted, daac.code_table(), text, 2);
    speed_test(stlmap, daac.code_table(), text, 2);
    //speed_test(cuckoo, daac.code_table(), text, 2);
}
