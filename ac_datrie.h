#pragma once

#include "ac_match.hxx"
#include "ac_builder.h"

namespace ac {

    template < typename _StateId = uint32_t>
    class DoubleArray32 {
    public:
        using MatchStateId = _StateId;
        using MatchState = MatchStateId;
    protected:
        struct _state_t {
            _StateId base_ = 0;
            _StateId check = FROM_INVALID;
            _StateId fail = 0;
            _StateId match = 0;
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
    protected:
        size_t    size_ = 0;
        std::vector<_state_t> states_;
        std::vector<_match_t> matches_;
        size_t    match_size_ = 0;
    public:
        MatchState     get_state(MatchStateId id) const {
            return (id < size_) ? id : 0;
        }
        MoveRet  move_state(MatchState& from, uint8_t code) const {
            const _state_t& from_ = states_[from];
            MatchStateId to = from_.base() + code;
            bool success = (to < size_) && (states_[to].check == from);
            if (success) {
                from = to;
                return SM_ACCEPTED;
            }
            bool from_root = from == 0;
            if (from_root) return SM_REACH_ROOT;
            from = from_.fail;
            return SM_FAIL_LINK;
        }
        bool        is_final(const MatchState& state) const {
            return states_[state].is_final();
        }
        MatchStateId  id_of(const MatchState& state) const {
            return state;
        }
    public:
        bool resize(size_t state_count, size_t valid_count, size_t matches) {
            size_ = state_count;
            states_.resize(state_count);
            matches_.resize(matches);
            return true;
        }
        bool add_state(size_t id, const ac::State& state, size_t mc, const ac::MatchPair* matches) {
            states_[id].set_base(state.base);
            states_[id].check = state.check;
            states_[id].fail = state.fail;
            states_[id].set_final(state.attr & S_FINAL);
            states_[id].match = mc ? match_size_ : 0;
            for (size_t i = 0; i < mc; ++i, match_size_++) {
                matches_[match_size_] = matches[i].kwidx;
            }
            return true;
        }
        bool done() {
            return true;
        }
        size_t room() const {
            return states_.size() * sizeof(_state_t) + 
                matches_.size() * sizeof(_match_t);
        }
    };

};