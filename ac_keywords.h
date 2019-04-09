#pragma once
#include <stdint.h>
#include <vector>
#include <string>

namespace ac {

    struct Keyword {
        uint32_t    index_ = 0;
        const char* data_ = nullptr;
        uint32_t    size_ = 0;
        Keyword() : data_(0), size_(0), index_(0) {
        }
        Keyword(const char* ptr) : data_(ptr), size_(ptr ? strlen(ptr) : 0) {
        }
        Keyword(const char* ptr, size_t len) : data_(ptr), size_(len) {
        }
        Keyword(const std::string& str) {
            data_ = str.c_str();
            size_ = str.size();
        }
        const char* data() const {
            return data_;
        }
        const char* c_str() const {
            return data_;
        }
        size_t size() const {
            return size_;
        }
        char operator [] (size_t i) const {
            return data_[i];
        }
        uint32_t index() const {
            return index_;
        }
        // 和std::string的行为一致
        int compare(const Keyword& right, bool ignore_case = false) const {
            size_t cmp_size = size_ < right.size_ ? size_ : right.size_;
            int diff = ignore_case ?
                _strnicmp(data_, right.data_, cmp_size) :
                strncmp(data_, right.data_, cmp_size);
            if (diff) return diff;
            if (size_ < right.size_)
                return -1;
            if (size_ > right.size_)
                return 1;
            return 0;
        }
        struct less_compare_t {
            bool ignore_case_ = false;
            less_compare_t(bool ignore_case) : ignore_case_(ignore_case) {
            }
            bool operator()(const Keyword& l, const Keyword& r) const {
                return l.compare(r, ignore_case_) < 0;
            }
        };
    };

    using keyword_t = Keyword;

    typedef std::vector<keyword_t> keywords_t;

    static void fill_index( keywords_t & kws ) {
        for( size_t i = 0; i < kws.size(); ++ i ) {
            kws[i].index_ = i;
        }
    }

};