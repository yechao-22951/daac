#pragma once

#include "ac_comm.h"

namespace ac {

    typedef enum STATE_MOVE_RESULT_ {
        SM_ACCEPTED,
        SM_FAIL_LINK,
        SM_REACH_ROOT,
    } MoveRet;

    template < typename MatchStateId, typename MatchState>
    struct Machine {
        MatchState      get_state(MatchStateId) const;
        MoveRet         move_state(MatchState&, uint8_t ch) const;
        bool            is_final(const MatchState&) const;
        MatchStateId    id_of(const MatchState&) const;
    };

    template <typename Machine = Machine,
        typename MatchStateId = typename Machine::MatchStateId,
        typename MatchState = typename Machine::MatchState >

        static bool match(const Machine & machine,
            const uint8_t * code_table,
            MatchStateId & state,
            mem_bound_t & buffer,
            bool verbose)
    {
        const uint8_t* data = buffer.data;
        const uint8_t* tail = buffer.tail;
        MatchState p = machine.get_state(state);
        for (; data < tail; ++data) {
            uint8_t b8 = *data;
            uint8_t ch = code_table[b8];
            for (;;) {
                MoveRet ms = machine.move_state(p, ch);
                if (ms == SM_ACCEPTED)
                {
                    if (machine.is_final(p)) {
                        state = machine.id_of(p);
                        buffer.data = data + 1;
                        return true;
                    }
                    break;
                }
                else if (ms == SM_FAIL_LINK) {
                    continue;
                }
                else {
                    break;
                }
            }
        }
        state = machine.id_of(p);
        buffer.data = data;
        return false;
    }

    template <typename Machine = Machine,
        typename MatchStateId = typename Machine::MatchStateId,
        typename MatchState = typename Machine::MatchState >

        static bool feed(const Machine & machine,
            const uint8_t * code_table,
            MatchStateId & state,
            uint8_t uch8,
            bool verbose)
    {
        MatchState p = machine.get_state(state);
        uint8_t ch = code_table[uch8];
        if (verbose) printf("\n");
        for (;;) {
            if (verbose) printf("%d ", machine.id_of(p));
            MoveRet ms = machine.move_state(p, ch);
            if (ms == SM_ACCEPTED)
            {
                if (machine.is_final(p)) {
                    state = machine.id_of(p);
                    return true;
                }
                break;
            }
            else if (ms == SM_FAIL_LINK) {
                continue;
            }
            else {
                break;
            }
        }
        state = machine.id_of(p);
        return false;
    }

};