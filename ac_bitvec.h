
#pragma once

#include "ac_match.hxx"
#include "ac_builder.h"
#include "rankable_bit_vector.hpp"

namespace ac {

    template < typename _StateId = StateId>
    class SuccinctArray {
    protected:
        struct _state_t {
            _StateId base_;
            _StateId check;
            _StateId fail;
            _StateId match;
            inline _StateId base() const {
                return BaseAcc::get_value(base_);
            }
            inline _StateId is_final() const {
                return BaseAcc::is_final(base_);
            }
            inline void set_base(size_t base) {
                base_ = base;
            }
            inline void set_final(size_t any) {
                if (any) BaseAcc::set_final(base_);
            }
        };
        using _match_t = _StateId;
        struct _match_state_t {
            _StateId id;
            const _state_t* s;
        };
    protected:
        succinct::dense::BitVector bitmap_;
        std::vector<_state_t> states_;
        std::vector<_match_t> matches_;
        size_t states_size_ = 0;
    public:
        // for ac_match
        using MatchStateId = _StateId;
        using MatchState = _match_state_t;
        // for 
        MatchState     get_state(MatchStateId id) const {
            if (!id || id >= states_size_)
                return { 0, &states_[0] };
            return  { id, &states_[bitmap_.rank(id)] };
        }
        inline MoveRet  move_state(MatchState & current, uint8_t code) const {
            bool from_root = current.id == 0;
            MatchStateId to = current.s->base() + code;
            bool success = to < states_size_ ? bitmap_.lookup(to) : false;
            const _state_t* target = nullptr;
            if (success) {
                target = &states_[bitmap_.rank(to)];
                success = (target->check == current.id);
            }
            if (!success && from_root) {
                current.s = &states_[0];
                return SM_REACH_ROOT;
            }
            if (!success) {
                to = current.s->fail;
                target = &states_[to ? bitmap_.rank(to) : 0];
            }
            current.id = to;
            current.s = target;
            return success ? SM_ACCEPTED : SM_FAIL_LINK;
        }
        bool        is_final(const MatchState & state) const {
            return state.s->is_final();
        }
        MatchStateId  id_of(const MatchState & state) const {
            return state.id;
        }

    public:

    public:
        bool resize(size_t state_count, size_t valid_count, size_t match_count) {
            states_size_ = state_count;
            bitmap_.init(state_count);
            states_.reserve(valid_count);
            matches_.reserve(match_count);
            return true;
        }
        bool add_state(size_t id, const ac::State & state, size_t mc, const ac::MatchPair * matches) {
            _state_t state_;
            state_.base_ = state.base;
            state_.check = state.check;
            state_.fail = state.fail;
            state_.set_final(state.attr & S_FINAL);
            state_.match = mc ? matches_.size() : 0;
            for (size_t i = 0; i < mc; ++i) {
                matches_.push_back(matches[i].kwidx);
            }
            bitmap_.set_bit(id, 1);
            states_.push_back(state_);
            return true;
        }
        bool done() {
            bitmap_.build_rank();
            return true;
        }
        size_t room() const {
            return bitmap_.room() +
                states_.size() * sizeof(_state_t) + 
                matches_.size() * sizeof(_match_t);
        }
    };

};



//
//class unordered_map_t {
//    using state_hash_table_t = std::unordered_map<st_idx_t, state_t>;
//    state_hash_table_t states_;
//public:
//    using state_t = daac_t::state_t;
//    using id_t = daac_t::id_t;
//    using State = daac_t::State;
//    // for make
//    void resize(size_t state_count, size_t valid_count) {
//        states_.clear();
//        states_.reserve(valid_count);
//    }
//    void add_state(id_t state_id, const state_t& state) {
//        states_[state_id] = state;
//    }
//    void done() {
//    }
//    // for match
//    inline size_t size() const {
//        return states_.size();
//    }
//
//    __forceinline bool move_to(State& from, uint8_t code) const {
//        bool from_root = from.id == 0;
//        id_t to = from.state->base + code;
//        auto it = states_.find(to);
//        bool success = it != states_.end();
//        const state_t* state = 0;
//        if (success) {
//            state = &it->second;
//            success = state->check == from.id;
//        }
//        if (!success && from_root) {
//            from.state = 0;
//            return false;
//        }
//        if (!success) {
//            to = from.state->fail;
//            state = &(states_.find(to)->second);
//        }
//
//        from.id = to;
//        from.state = state;
//
//        return success;
//
//    }
//
//    __forceinline State set_to(id_t to) const {
//        auto it = states_.find(to);
//        const state_t* state = &it->second;
//        return State{ to, state };
//    }
//
//    // 
//    size_t room() const {
//        return 0;
//    }
//};
