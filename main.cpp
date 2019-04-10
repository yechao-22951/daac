#include <stdio.h>
#include "mp_sm_mini_ac_builder.h"
#include <random>
#include <deque>
#include <iostream>
#include <fstream>
#include "rankable_bit_vector.hpp"
#include <unordered_map>
#include "ac_builder.h"
#include "ac_bitvec.h"
#include "ac_datrie.h"
#include "ac_uomap.h"
#include "sm_descape.h"

std::random_device generator;

#include "ac_match.hxx"
#include "ac_utils.h"

int main() {

    //mp::NestUnescape decoder;
    //std::string xxx = "AA%u\\x41\\x41bb ";
    //for( uint8_t ch : xxx ) {
    //    decoder.push_pop(ch);
    //}

    ac::DoubleArrayBuilder dab;

    std::ifstream file;
    file.open("find.txt");
    std::deque<std::string> lines;
    while (!file.eof()) {
        std::string line;
        std::getline(file, line);
        if (line.empty())
            continue;
        lines.emplace_back(line);
    };
    file.close();

    ac::keywords_t kws;
    kws.reserve(lines.size());
    for (auto& l : lines) {
        kws.push_back(l);
    }

    dab.build(kws,0);

    ac::SuccinctArray<> sa;
    ac::DoubleArray32 da;
    ac::HashTable<> ht;
    dab.make(sa);
    dab.make(da);
    dab.make(ht);

    std::string text;
    text.resize(1024 * 1024);

    for (auto& ch : text) {
        ch = '%' + (generator() % 80);
    }

    const size_t R = 100, TIMES = 10;
    for( size_t i = 0; i < TIMES; ++ i ) {
        {
            mp::NestUnescape decoder;
            double start = GetTickCount();
            size_t ALL = 0;
            size_t total = text.size() * R;
            ac::mem_bound_t mb;
            ac::mem_bound_init(mb,text);
            for( size_t r = 0; r < R; ++ r ) {
                ac::mem_bound_t scan = mb;
                while( ac::mem_bound_size(scan) ) {
                    ALL += decoder.push_pop(scan);
                }
            }
            double durtion = GetTickCount() - start;
            printf("decoder(if-else), durtion: %zd ms, decode: %zd, speed; %0.2f MB/s, decoded: %zd\n",
                (size_t)durtion, total, total / durtion * 1000 / 1024 / 1024, ALL);
        }
        {
            mp::NestUnescape decoder;
            double start = GetTickCount();
            size_t ALL = 0;
            size_t total = text.size() * R;
            ac::mem_bound_t mb;
            ac::mem_bound_init(mb, text);
            for (size_t r = 0; r < R; ++r) {
                ac::mem_bound_t scan = mb;
                while (ac::mem_bound_size(scan)) {
                    ALL += decoder.push_pop_2(scan);
                }
            }
            double durtion = GetTickCount() - start;
            printf("decoder(switch), durtion: %zd ms, decode: %zd, speed; %0.2f MB/s, decoded: %zd\n",
                (size_t)durtion, total, total / durtion * 1000 / 1024 / 1024, ALL);
        }
    }

    return 0;

    text += "/home/joy/.wine/drnive_c/windows/system32/api-ms-win-core-kernel32-private-l1-1-1.dll";

    size_t c = ac::hit_count(text, std::string("yec"));
    printf( "count = %d\n", c);

    bool same = ac::consistence_check(sa, da, dab.code_table(), text, 1);
    printf("%d\n", same);

    speed_test(sa, dab.code_table(), text, 50);
    speed_test(da, dab.code_table(), text, 50);
    speed_test(ht, dab.code_table(), text, 50);
}
