#pragma once
#include <stdint.h>
#include <deque>

namespace mp {

    enum {
        bc_skip,
        bc_either,
        bc_repeat,
        bc_search_1,
        bc_search_n,
        bc_match_1,
        bc_match_n,
    };

    namespace sm {

        class matcher_t;

        typedef union {

            struct {
                uint32_t rest;
            } jump_;

            struct {
                uint32_t rest;
                int32_t offset;
            } repeat_;

            struct {
                uint32_t rest;
                uint32_t state;
            } bc_search_1;

            struct {
                uint32_t rest;
                uint32_t state;
            } bc_search_n;

            struct {
                uint32_t rest;
                uint32_t state;
            } bc_match_1;

            struct {
                uint32_t rest;
                uint32_t state;
            } bc_match_n;



        } un_state_t;

        struct bc_bound_t {
            uint16_t head;
            uint16_t tail;
        };

        struct routine_t {
            routine_t* parent;
            uint8_t kind_;
            bc_bound_t code;
            un_state_t locals_;
            int execute(uint8_t ch) {
                return 0;
            }
        };

        class machine_t {
            routine_t* current_;
            bc_bound_t program_;
        public:
            void mro_delete(routine_t* p) {

            }
            void mro_push(uint8_t kind) {

            }
            void mro_return() {
                routine_t* del = std::exchange(root, root->parent);
                mro_delete(del);
            }
            int execute_(uint8_t ch) {
                if (current_) {
                    un_state_t& lvs = current_->locals_;
                    switch (root_->kind_) {
                    case bc_skip:
                        if (!--lvs.jump_.rest)
                            return 1;
                        break;
                    case bc_repeat:
                        if (!--lvs.repeat_.rest)
                            return 1;
                        program_.head += lvs.repeat_.offset;
                    }
                }
                if (r == 0) {
                    mro_return();
                }
            }
            inline int do_jump16() {

            }
        };



    };


};