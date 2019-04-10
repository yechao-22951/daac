#pragma once
#include <string>
#include <Windows.h>
#include "ac_match.h"

namespace ac {
    template <typename T >
    void speed_test(const T& searcher, const std::string& text, size_t loop) {
        const char* class_name = typeid(T).name();
        typename T::MatchStateId state = 0;
        double total = text.size() * loop;
        ac::mem_bound_t in;
        size_t matched = 0;
        size_t start = GetTickCount();
        for (size_t k = 0; k < loop; ++k) {
            ac::mem_bound_init(in, text);
            while (ac::mem_bound_size(in)) {
                matched += ac::match(searcher, state, in, false) ? 1 : 0;
            }
        }
        double durtion = GetTickCount() - start;
        printf("%s, machine-space: %zd KB, durtion: %zd ms, scan: %0.2f MB, speed; %0.2f Mb/s, matched: %zd\n",
            class_name, searcher.room()/1024, (size_t)durtion, total/1024/1024, total / durtion * 1000 / 1024 / 1024 * 8, matched);
    }
    template <typename T, typename K >
    bool consistence_check(const T& searcher1, const K& searcher2, const std::string& text, size_t loop) {
        typename T::MatchStateId state1 = 0;
        typename K::MatchStateId state2 = 0;
        size_t in_size = text.size();
        size_t matched1 = 0, matched2 = 0;
        for (size_t k = 0; k < loop; ++k) {
            for( size_t i = 0; i < in_size; ++ i ) {
                uint8_t uch = text[i];
                typename T::MatchStateId state1_ = state1;
                typename K::MatchStateId state2_ = state2;
                bool verbose = false;
                for(;;) {
                    matched1 += ac::feed(searcher1, state1, uch, verbose) ? 1 : 0;
                    matched2 += ac::feed(searcher2, state2, uch, verbose) ? 1 : 0;
                    if( matched1 != matched2 || state1 != state2 ) {
                        state1 = state1_;
                        state2 = state2_;
                        verbose = true;
                        continue;
                    } else {
                        break;
                    }
                };
            }
        }
        return true;
    }

    size_t hit_count( const std::string & text, const std::string & find ) {
        size_t count = 0;
        size_t pos = 0;
        for(;; count++) {
            auto off = text.find( find, pos);
            if( off == std::string::npos)
                break;
            pos = off + 1;
        }
        return count;
    }
}