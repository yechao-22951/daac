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
#include "mp_script.h"

std::random_device generator;

#include "ac_match.h"
#include "ac_utils.h"


template < typename Decoder, typename Searcher>
void DecodeScan(Decoder& decoder, Searcher& searcher, ac::mem_bound_t& in, size_t loop) {
    size_t matched = 0, searched = 0, input = ac::mem_bound_size(in) * loop;
    double start = GetTickCount();
    typename Searcher::MatchStateId match_state = 0;
    for (size_t r = 0; r < loop; ++r) {
        ac::mem_bound_t mb = in;
        while (ac::mem_bound_size(mb)) {
            size_t c = decoder.forward(mb);
            if (!c) continue;
            ac::mem_bound_t mbs;
            ac::mem_bound_init(mbs, decoder.data(), c);
            searched += c;
            while (ac::mem_bound_size(mbs)) {
                matched += ac::match(searcher, match_state, mbs, false) ? 1 : 0;
            }
        }
    }
    size_t c = decoder.finsh();
    if (c) {
        ac::mem_bound_t mbs;
        ac::mem_bound_init(mbs, decoder.data(), c);
        searched += c;
        while (ac::mem_bound_size(mbs)) {
            matched += ac::match(searcher, match_state, mbs, false) ? 1 : 0;
        }
    }
    double durtion = GetTickCount() - start;
    size_t speed = input / durtion * 1000 / 1024 / 1024;
    printf("DecodeScan, durtion: %d ms, input: %d MB, searched: %d MB, speed: %d MB/s, matched: %d\n",
        (int)durtion, (int)input / 1024 / 1024, (int)searched / 1024 / 1024, (int)speed, (int)matched);
}

int main() {

    ac::DoubleArrayBuilder dab;

    std::ifstream file;
    file.open("prefixes.txt");
    std::deque<std::string> lines;
    while (!file.eof()) {
        std::string line;
        std::getline(file, line);
        if (line.empty())
            continue;
        lines.emplace_back(line);
    };
    file.close();

    //////////////////////////

    ac::keywords_t kws;
    kws.reserve(lines.size());
    for (auto& l : lines) {
        kws.push_back(l);
    }

    kws = {"yechao", "hello"};

    dab.build(kws, 0);
    
    //////////////////////////

    ac::SuccinctArray<> sa;
    dab.make(sa);

    //////////////////////////
    std::string text;

    //text.resize(1024 * 1024);
    //for (auto& ch : text) {
    //    ch = '%' + (generator() % 0x100);
    //}

    text += "\\x%37%39\\x65\\x63\\x68\\x61\\x6F";

    //////////////////////////
    mp::normalize_t<64> normalize;
    mp::operator_t<64>  operators;
    mp::alphanum_t<64>  alphanum;

    mp::forward_match_t matcher1(sa);
    mp::forward_match_t matcher2(sa);
    mp::forward_match_t matcher3(sa);

    mp::forward_chain_t chain1(normalize, matcher1);
    mp::forward_chain_t chain2(operators, matcher2);
    mp::forward_chain_t chain3(alphanum, matcher3);

    {
        mp::speed_test(chain1, text, 100);
        mp::speed_test(chain2, text, 100);
        mp::speed_test(chain3, text, 100);
    }
    //for (;; ) {
    //    ac::mem_bound_t mb;
    //    ac::mem_bound_init(mb, text);
    //    DecodeScan(decoder, da, mb, 100);
    //}

    text += "/home/joy/.wine/drnive_c/windows/system32/api-ms-win-core-kernel32-private-l1-1-1.dll";

    size_t c = ac::hit_count(text, std::string("yec"));
    printf("count = %d\n", c);

    bool same = ac::consistence_check(sa, sa, text, 1);
    printf("%d\n", same);

    for (size_t L = 0; L < 10; ++L) {
        speed_test(sa, text, 50);
        //speed_test(da, text, 50);
        //speed_test(ht, dab.code_table(), text, 50);
    }
}
