#pragma once
#include <stdint.h>
#include <unordered_map>
#include "ac_comm.h"
#include "ac_match.h"
#include "ac_builder.h"

namespace ac
{
    template < typename _StateId = StateId>
    class HashTable : public WithCodeTable {
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
        typedef  std::unordered_map<_StateId, _state_t> StateTable; 
        StateTable states_;
        std::vector<_match_t> matches_;
        size_t states_size_ = 0;
    protected:
        const _state_t* __find_state(_StateId id) const {
            if (id >= states_size_) return NULL;
            auto it = states_.find(id);
            if (it == states_.end()) return NULL;
            return &it->second;
        }
    public:
        // for ac_match
        using MatchStateId = _StateId;
        using MatchState = _match_state_t;
        // for 
        MatchState     get_state(MatchStateId id) const {
            if (!id || id >= states_size_)
                return { 0, __find_state(0) };
            return  { id, __find_state(id) };
        }
        inline MoveRet  move_state(MatchState & current, uint8_t code) const {
            bool from_root = current.id == 0;
            MatchStateId to = current.s->base() + code;
            const _state_t* target = __find_state(to);
            bool success = !!target;
            if (success) {
                success = (target->check == current.id);
            }
            if (!success && from_root) {
                current.s = __find_state(0);
                return SM_REACH_ROOT;
            }
            if (!success) {
                to = current.s->fail;
                target = __find_state(to);
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
            states_[id] = state_;
            return true;
        }
        bool done() {
            return true;
        }
        size_t room() const {
            const size_t bucket_count = states_.bucket_count();
             size_t hash_table_size = 0;
            for( size_t i = 0; i < bucket_count; ++ i ) {
                hash_table_size += states_.bucket_size(i) * sizeof(StateTable::value_type);
            }
            return hash_table_size + matches_.size() * sizeof(_match_t);
        }
    };
};

