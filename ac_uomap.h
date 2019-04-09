#pragma once

namespace ac {

    class unordered_map_t {
        using state_hash_table_t = std::unordered_map<state_id_t, _state_t>;
        state_hash_table_t states_;
    public:
        using _state_t = daac_t::_state_t;
        using state_id_t = daac_t::state_id_t;
        using State = daac_t::State;
        // for make
        void resize(size_t state_count, size_t valid_count) {
            states_.clear();
            states_.reserve(valid_count);
        }
        void add_state(state_id_t state_id, const _state_t& state) {
            states_[state_id] = state;
        }
        void done() {
        }
        // for match
        inline size_t size() const {
            return states_.size();
        }

        __forceinline bool move_to(State& from, uint8_t code) const {
            bool from_root = from.id == 0;
            state_id_t to = from.state->base + code;
            auto it = states_.find(to);
            bool success = it != states_.end();
            const _state_t* state = 0;
            if (success) {
                state = &it->second;
                success = state->check == from.id;
            }
            if (!success && from_root) {
                from.state = 0;
                return false;
            }
            if (!success) {
                to = from.state->fail;
                state = &(states_.find(to)->second);
            }

            from.id = to;
            from.state = state;

            return success;

        }

        __forceinline State set_to(state_id_t to) const {
            auto it = states_.find(to);
            const _state_t* state = &it->second;
            return State{ to, state };
        }

        // 
        size_t room() const {
            return 0;
        }
    };

};