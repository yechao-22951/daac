#pragma once

#include <stdint.h>
#include <Windows.h>
#include <stack>
#include <functional>
#include "ac_comm.h"

namespace CharTrait {
    enum
    {
        Lowcase = 1,      // LOW-CASE
        Upcase = 2,      // UP-CASE
        Alpha = Lowcase | Upcase,
        OpChar = 4,      // Operator
        OctChar = 8,      // oct-digt
        HexChar = 16,     // hex
        DecChar = 32,     // dec/digital
        BinChar = 64,     // bin
        Escape = 128,    // Escape

        OPTR = OpChar,
        DECN = DecChar | HexChar,
        OCTN = OctChar | DecChar | HexChar,
        BINN = BinChar | OctChar | DecChar | HexChar,
        ESCP = Escape,
        ALPL = Lowcase,
        ALPU = Upcase,
        ALOP = OPTR | Alpha,
        UAHC = Upcase | HexChar,
        LAHC = Lowcase | HexChar,
        ESCO = Escape | OPTR,
    };

    static const uint32_t Table[256] = {
        0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      ,
        0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      ,
        0x00      , OPTR      , OPTR      , OPTR      , OPTR      , ESCO      , OPTR      , OPTR      , OPTR      , OPTR      , OPTR      , OPTR      , OPTR      , OPTR      , ALOP      , OPTR      ,
        BINN      , BINN      , OCTN      , OCTN      , OCTN      , OCTN      , OCTN      , OCTN      , DECN      , DECN      , OPTR      , OPTR      , OPTR      , OPTR      , OPTR      , OPTR      ,
        OPTR      , UAHC      , UAHC      , UAHC      , UAHC      , UAHC      , UAHC      , ALPU      , ALPU      , ALPU      , ALPU      , ALPU      , ALPU      , ALPU      , ALPU      , ALPU      ,
        ALPU      , ALPU      , ALPU      , ALPU      , ALPU      , ALPU      , ALPU      , ALPU      , ALPU      , ALPU      , ALPU      , OPTR      , ESCO      , OPTR      , OPTR      , OPTR      ,
        OPTR      , LAHC      , LAHC      , LAHC      , LAHC      , LAHC      , LAHC      , ALPL      , ALPL      , ALPL      , ALPL      , ALPL      , ALPL      , ALPL      , ALPL      , ALPL      ,
        ALPL      , ALPL      , ALPL      , ALPL      , ALPL      , ALPL      , ALPL      , ALPL      , ALPL      , ALPL      , ALPL      , OPTR      , OPTR      , OPTR      , OPTR      , OPTR      ,
        0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      ,
        0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      ,
        0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      ,
        0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      ,
        0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      ,
        0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      ,
        0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      ,
        0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00
    };
};

class decoder_t {
public:
    enum state_t {
        escape_ = '\\',

        escape_x,
        escape_x_1,
        escape_x_2,
        escape_o_1,
        escape_o_2,
        escape_u,
        escape_u_1,
        escape_u_2,
        escape_u_3,
        escape_u_4,

        unescape_ = '%',
        unescape_x_1,
        unescape_x_2,
        unescape_u,
        unescape_u_1,
        unescape_u_2,
        unescape_u_3,
        unescape_u_4
    };
public:
    enum { MAX_NEST = 16, MAX_CBS = 64 };
    uint32_t            states_[MAX_NEST];
    uint8_t             utcc_[MAX_CBS];
    uint32_t            i = 0;
    uint32_t            nested_ = 0;
#ifdef _DEBUG
#define CHECK_CHAR_STACK()  if( (int32_t)i < 0 ) DebugBreak();
#else
#define CHECK_CHAR_STACK()  
#endif
public:
    static uint8_t o2d(uint8_t o) {
        return o - '0';
    }
    static uint8_t h2d(uint8_t h) {
        if (h & 0x10) return h - '0';
        if (h & 0x20) return h - 'a' + 10;
        return h - 'A' + 10;
    }
    uint16_t pop_hex_4(size_t prefix_size) {
        uint16_t r =
            (h2d(utcc_[i - 4]) << 12) +
            (h2d(utcc_[i - 3]) << 8) +
            (h2d(utcc_[i - 2]) << 4) +
            (h2d(utcc_[i - 1]));
        i -= 4 + prefix_size;
        CHECK_CHAR_STACK();
        return r;
    }
    uint8_t pop_hex_2(size_t prefix_size) {
        uint8_t r =
            (h2d(utcc_[i - 2]) << 4) +
            (h2d(utcc_[i - 1]));
        i -= 2 + prefix_size;
        CHECK_CHAR_STACK();

        return r;
    }
    uint8_t pop_oct_3(size_t prefix_size) {
        uint8_t r =
            (o2d(utcc_[i - 3]) << 6) +
            (o2d(utcc_[i - 2]) << 3) +
            (o2d(utcc_[i - 1]));
        i -= 3 + prefix_size;
        CHECK_CHAR_STACK();

        return r;
    }

    // FIXME: 大小字节序
    struct byte_queue_64_t {
        uint64_t queue_ = 0;
        size_t size_ = 0;
        //byte_queue_64_t(uint8_t b) {
        //    queue_ = b;
        //    size_ = 1;
        //}
        bool enqueue(uint16_t u16) {
            if (size_ + 2 >= sizeof(uint64_t))
                return false;
            queue_ |= u16 << (size_ * 8); size_ += 2;
            return true;
        }
        bool enqueue(uint8_t ch) {
            if (size_ >= sizeof(uint64_t))
                return false;
            queue_ |= ch << (size_ * 8); size_++;
            return true;
        }
        bool dequeue(uint8_t & ch) {
            if (!size_) return false;
            // get low-byte, and right-shift
            ch = queue_ & 0xff; queue_ >>= 8;
            --size_;
            return true;
        }
        size_t size() {
            return size_;
        }
    };

    static bool enqueue_byte(uint64_t & queue, size_t & qsize, uint8_t ch) {
        queue |= ch << (qsize * 8); qsize++;
    }
    static bool dequeue_byte(uint64_t & queue, size_t & qsize, uint8_t ch) {
        queue |= ch << (qsize * 8); qsize++;
    }

    void pop_state() {
        --nested_;
    }

    void pop_last_char() {
        --i;
        CHECK_CHAR_STACK();
    }

    void push_char(const uint8_t ch) {
        utcc_[i++] = ch;
    }

    void replace_state(size_t state) {
        states_[nested_ - 1] = state;
    }
    void enter_state(size_t state, bool replace = false) {
        if (replace) states_[nested_] = state;
        else states_[nested_++] = state;
    }

    inline void hanle_one_byte(const uint8_t ch, byte_queue_64_t & queue) {
        const uint8_t tr = CharTrait::Table[ch];
        if (!tr) {
            nested_ = 0;
            return;
        }
        push_char(ch);
        for (;;) {
            // 在任何状态，遇到escape/unescape,都可能存在转义
            if (tr & CharTrait::Escape) {
                enter_state(ch);
                return;
            }
            if (!nested_) {
                return;
            }
            size_t state = states_[nested_ - 1];
            if (state == escape_) {
                if (ch == 'x') {
                    replace_state(escape_x);
                    return;
                }
                else if (ch == 'u') {
                    replace_state(escape_u);
                    return;
                }
                else if (ch == '\\') {
                    pop_last_char();        // pop '\'
                    return;
                }
                else if (tr & CharTrait::OctChar) {
                    replace_state(escape_o_1);
                    return;
                }
            }
            else if (state == unescape_) {
                if (tr & CharTrait::HexChar) {
                    replace_state(unescape_x_1);
                    return;
                }
                else if (ch == 'u') {
                    replace_state(unescape_u);
                    return;
                }
            }
            else if (state == escape_o_1) {
                if (tr & CharTrait::OctChar) {
                    replace_state(state + 1);
                    return;
                }
            }
            else if (state == escape_o_2) {
                if (tr & CharTrait::OctChar) {
                    // \ ..
                    uint8_t nc = pop_oct_3(1);
                    queue.enqueue(nc);
                    pop_state();
                    return;
                }
            }
            else if (state == escape_x ||
                state == escape_u ||
                state == escape_u_1 ||
                state == escape_u_2 ||
                state == unescape_u ||
                state == unescape_u_1 ||
                state == unescape_u_2)
            {
                if (tr & CharTrait::HexChar) {
                    replace_state(state + 1);
                    return;
                }
            }
            else if (state == escape_x_1) {
                if (tr & CharTrait::HexChar) {
                    // \x
                    uint8_t nc = pop_hex_2(2);
                    queue.enqueue(nc);
                    pop_state();
                    return;
                }
            }
            else if (state == unescape_x_1) {
                if (tr & CharTrait::HexChar) {
                    // %
                    uint8_t nc = pop_hex_2(1);
                    queue.enqueue(nc);
                    pop_state();
                    return;
                }
            }
            else if (state == escape_u_3 || state == unescape_u_3) {
                if (tr & CharTrait::HexChar) {
                    // \u %u
                    uint16_t nc2 = pop_hex_4(2);
                    queue.enqueue(nc2);
                    pop_state();
                    return;
                }
            }
            else {
                int a = 1;
            }
            pop_state();
        }
    }

    inline size_t push_pop(uint8_t b8) {
        byte_queue_64_t queue;
        do {
            hanle_one_byte(b8, queue);
        } while (queue.dequeue(b8));
        if (!nested_) {
            if (i >= MAX_CBS / 2) {
                size_t c = i;
                i = 0;
                return c;
            }
        }
        return 0;
    }

    inline size_t push_pop(ac::mem_bound_t & mb) {
        for (; mb.data < mb.tail; ++mb.data) {
            uint8_t ch = *mb.data;
            byte_queue_64_t queue;
            do {
                hanle_one_byte(ch, queue);
            } while (queue.dequeue(ch));
            if (!nested_) {
                if (i >= MAX_CBS / 2) {
                    size_t c = i;
                    i = 0;
                    return c;
                }
            }
        }
        return 0;

    }

    inline size_t push_pop_2(ac::mem_bound_t & mb) {
        byte_queue_64_t queue;
        for (; mb.data < mb.tail; ++mb.data) {
            uint8_t ch = *mb.data;
            // for new-char
            do {
                const uint8_t tr = CharTrait::Table[ch];
                if (!tr) {
                    nested_ = 0;    // 强制清空所有状态
                    continue;
                }
                push_char(ch);

                // TR !=== 0
                for (;;) {
                    // for same char
                    // 在任何状态，遇到escape/unescape,都可能存在转义
                    if (tr & CharTrait::Escape) {
                        enter_state(ch);
                        break;
                    }
                    if (!nested_) {
                        // just push_char
                        // break;
                        break;
                    }
                    size_t state = states_[nested_ - 1];
                    switch (state) {
                    case escape_:
                        if (ch == 'x') {
                            replace_state(escape_x);
                        }
                        else if (ch == 'u') {
                            replace_state(escape_u);
                        }
                        else if (ch == '\\') {
                            pop_last_char();        // pop '\'
                            pop_state();
                        }
                        else if (tr & CharTrait::OctChar) {
                            replace_state(escape_o_1);
                        }
                        else {
                            pop_state();
                            continue;
                        }
                        break;
                    case unescape_:
                        if (tr & CharTrait::HexChar) {
                            replace_state(unescape_x_1);
                        }
                        else if (ch == 'u') {
                            replace_state(unescape_u);
                        }
                        else {
                            pop_state();
                            continue;
                        }
                        break;
                    case escape_o_1:
                        if (tr & CharTrait::OctChar) {
                            replace_state(state + 1);
                        }
                        else {
                            pop_state();
                            continue;
                        }
                        break;
                    case escape_o_2:
                        if (tr & CharTrait::OctChar) {
                            uint8_t nc = pop_oct_3(2);
                            queue.enqueue(nc);
                        }
                        pop_state();
                        break;

                    case escape_x:
                    case escape_u: case escape_u_1: case escape_u_2:
                    case unescape_u: case unescape_u_1: case unescape_u_2:
                        if (tr & CharTrait::HexChar) {
                            replace_state(state + 1);
                        }
                        else {
                            pop_state();
                            continue;
                        }
                        break;
                    case escape_x_1: case unescape_x_1:
                        if (tr & CharTrait::HexChar) {
                            uint8_t nc = pop_hex_2(2);
                            queue.enqueue(nc);
                        }
                        pop_state();
                        break;
                    case escape_u_3: case unescape_u_3:
                        if (tr & CharTrait::HexChar) {
                            uint16_t nc2 = pop_hex_4(2);
                            queue.enqueue(nc2);
                        }
                        pop_state();
                        break;
                    }
                    break;
                }
            } while (queue.dequeue(ch));

            if (!nested_) {
                if (i >= MAX_CBS / 2) {
                    size_t c = i;
                    i = 0;
                    return c;
                }
            }
        }
        return 0;
    }
};