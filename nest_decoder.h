#pragma once

#include <stdint.h>
#include <stack>

class decoder_t {
public:
    enum state_t {
        normal,
        space,
        escape_x,
        escape_o,
        escape_u,
        unescape_x,
        unescape_u,
    };
public:
    std::deque<state_t> state_stack_;
    uint8_t             string_[16];
    uint8_t             strlen_;
public:
    decoder_() {
        state_stack_.push(normal);
    }
    size_t push_pop(const uint8_t* p, size_t len) {
        uint8_t ch = *p;
        while (state_stack_.size()) {
            state_t s = state_stack_.back();
            string_[strlen_++] = ch;
            switch (s) {
            case normal:
                if(strlen_ > 1 ) {
                    if( ch == 'x' || ch == 'u' ) {
                        uint8_t l2 = string_[strlen_-2];
                        if( l2 == '\\' ) {
                            state_stack_.push(escape_x);
                        }
                        if (l2 == '%') {
                            state_stack_.push(escape_x);
                        }
                    } else if(is_oct(ch)){
                        state_stack_.push(escape_x);
                    }
                }
            case escape_x:
                if( !is_hex(ch) ) {
                    
                }
            }
        }
    }
};