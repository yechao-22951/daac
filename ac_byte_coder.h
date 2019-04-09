#pragma once
#include <stdint.h>
#include <vector>
#include "ac_comm.h"
#include "ac_keywords.h"

namespace ac {
    static inline uint8_t lower(uint8_t ch) {
        if ('A' >= ch && 'Z' >= 'ch')
            return ch + 'a' - 'A';
        return ch;
    }
    static inline uint8_t _byte(uint8_t ch, uint8_t opt) {
        return opt & CASE_IGNORE ? lower(ch) : ch;
    }
    class byte_coder_t {
    protected:
        struct _stat_t {
            uint8_t ch = 0;
            size_t  count = 0;
            static bool less_by_count(const _stat_t& l, const _stat_t& r) {
                return l.count > r.count;
            }
        };
    protected:
        uint8_t table_[0x100] = {};
        uint8_t max_code_ = 0;
    public:
        byte_coder_t() {};
        uint8_t translate(uint8_t ch) const {
            return table_[ch];
        }
        uint8_t* table() {
            return table_;
        }
        uint8_t operator [] (uint8_t ch) const {
            return table_[ch];
        }
        size_t max_code() {
            return max_code_;
        }
        void clear() {
            memset(table_, 0, sizeof(table_));
        }
        void get_table(uint8_t* table) {
            memcpy(table, table_, sizeof(table_));
        }
        size_t build(const keywords_t & kws, uint8_t options) {
            size_t total = 0;
            std::vector<_stat_t> stats(0x100);
            for (size_t i = 0; i < stats.size(); ++i) {
                stats[i].ch = i;
                stats[i].count = 0;
            }
            max_code_ = 0;
            for (auto& kw : kws) {
                total += kw.size();
                for (size_t i = 0; i < kw.size(); ++i) {
                    uint8_t ch = _byte(kw[i], options);
                    if (!stats[ch].count) max_code_++;
                    stats[ch].count++;
                }
            }
            std::sort(stats.begin(), stats.end(), _stat_t::less_by_count);
            size_t code = 0;
            for (size_t i = 0; i < stats.size(); ++i) {
                uint8_t ch = stats[i].ch;
                if (stats[i].count) {
                    table_[ch] = code++;
                }
                else {
                    table_[ch] = max_code_;
                }
            }
            if (options & CASE_IGNORE) {
                for (uint8_t ch = 'A'; ch < 'Z'; ++ch) {
                    table_[ch] = table_[ch + 'a' - 'A'];
                }
            }
            return total;
        }
    };

};