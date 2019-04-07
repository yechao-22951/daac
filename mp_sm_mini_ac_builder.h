#pragma once

#include <vector>
#include "mp_sm_mini_ac.h"

namespace sm {
    namespace algo {

        struct keyword_t {
            const char* data_ = nullptr;
            uint32_t    size_ = 0;
            uint32_t    user_ = 0;
            // mSearch是纯粹的字符匹配，不需要带用户数据
            //uint32_t    user;
            keyword_t() : data_(0), size_(0) {
            }
            keyword_t(const char* ptr) : data_(ptr), size_(ptr ? strlen(ptr) : 0) {
            }
            keyword_t(const std::string& str) {
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
            uint32_t user_data() const {
                return user_;
            }
            // 和std::string的行为一致
            int compare(const keyword_t& right, bool cs = false) const {
                size_t cmp_size = size_ < right.size_ ? size_ : right.size_;
                int diff = cs ?
                    strncmp(data_, right.data_, cmp_size) :
                    _strnicmp(data_, right.data_, cmp_size);
                if (diff) return diff;
                if (size_ < right.size_)
                    return -1;
                if (size_ > right.size_)
                    return 1;
                return 0;
            }

            struct less_compare_t {
                bool case_sensitive = false;
                less_compare_t(bool cs = false) : case_sensitive(cs) {
                }
                bool operator()(const keyword_t& l, const keyword_t& r) const {
                    return l.compare(r, case_sensitive) < 0;
                }
            };
        };

        typedef std::vector<keyword_t> keywords_t;

        class skSearchBuilder {
        protected:
            using state_t = skSearch::state_t;
            std::vector<state_t> states_;
        public:
            skSearchBuilder() {
            }
            size_t dump() {
                return states_.size() * sizeof(state_t);
            }
            int build(const std::string_view& str) {
                size_t sl = str.size();
                if (sl > 0x100)
                    return -1;
                states_.resize(sl);
                states_[0] = {};

                for (size_t i = 0; i < sl; ++i) {
                    uint8_t c = str[i];
                    states_[i].accpet = c;
                    states_[i].fail = 0;
                    if (i > 0) {
                        size_t ifail = states_[i - 1].fail;
                        size_t itarget = (states_[ifail].accpet == c) ? ifail + 1 : 0;
                        states_[i].fail = itarget;
                    }
                }
                return 0;
            }
        };


        template < typename st_idx_t = uint16_t >
        class mkSearchBuilder {
        public:
            typedef mkSearch<st_idx_t> Searcher;
        protected:
            using  state_t = typename Searcher::state_t;
            struct level_info_t {
                std::vector<st_idx_t> state;
                level_info_t(size_t size) {
                    state.resize(size, 0);
                }
                void reset() {
                    state.resize(state.size(), 0);
                }
            };
        protected:
            std::vector<state_t> states_;
            state_t* root_ = nullptr;
            size_t alloced_ = 0;
            uint8_t options_ = 0;
        protected:
            size_t alloc_states_(const keywords_t& keywords) {
                size_t total = 0;
                size_t max_len = 0;
                for (size_t i = 0; i < keywords.size(); ++i) {
                    const keyword_t& kw = keywords[i];
                    size_t len = kw.size();
                    total += len;
                    if (max_len < len)
                        max_len = len;
                }
                states_.resize(1 + total);          // additional one is for root-state
                root_ = (state_t*)states_.data();
                alloced_ = 1;
                return max_len;
            }
            state_t* state_move(state_t * state, uint8_t ch) {
                return Searcher::move_state(root_, state, ch);
            }
            bool build_level(const keywords_t & keywords, size_t strpos, level_info_t * up_level, level_info_t * level) {
                for (size_t i = 0; i < keywords.size(); ++i)
                {
                    const keyword_t& kw = keywords[i];
                    state_t* parent = up_level->state[i] + root_;
                    if (strpos == kw.size()) {
                        if (parent->attr & B_FINAL) {
                            // mkSearch 不需要处理这种情况
                            // 但真正的AC需要处理 ***
                        }
                        else {
                            parent->attr |= B_FINAL;
                            // mkSearch 不需要 user
                            // 但真正的AC需要 ***
                            //parent->base = kw.user;
                        }
                        continue;
                    }
                    if (strpos >= kw.size()) {
                        continue;
                    }
                    uint8_t ch = (options_ & ACFF_CASE_SENSITIVE) ? kw[strpos] : ::tolower(kw[strpos]);

                    state_t* curr = state_move(parent, ch);
                    if (curr) {
                        // 更新本层该位置的状态
                        level->state[i] = curr - root_;
                    }
                    else {
                        // 现在可以确定parent的base了
                        if (parent->len++ == 0)
                            parent->base = alloced_;
                        // 创建新的状态
                        size_t new_state = alloced_++;
                        if (new_state != (st_idx_t)new_state)
                            return false;
                        // 更新本层该位置的状态
                        level->state[i] = new_state;
                        state_t * new_state_node = root_ + new_state;
                        new_state_node->code = ch;
                        // 设置失败节点
                        state_t * fail_link = parent->fail + root_;
                        while (fail_link != parent) {
                            state_t* target = state_move(fail_link, ch);
                            if (target) {
                                new_state_node->fail = target - root_;
                                // 设置等效匹配节点( mkSearch仅需设置FLAG)
                                for (; target != root_; target = target->fail + root_) {
                                    if (target->attr & B_FINAL) {
                                        //new_state_node->match = new_state_node->fail;
                                        new_state_node->attr |= B_HAS_MATCH;
                                        break;
                                    }
                                }
                                break;
                            }
                            // 找下一个失败节点
                            auto next_fail = fail_link->fail + root_;
                            if (next_fail == fail_link)
                                break;
                            fail_link = next_fail;
                        }
                    }
                }
                return true;
            }
        public:
            void clear() {
                alloced_ = 0;
                options_ = 0;
                root_ = 0;
                states_.clear();
            }
            Searcher* make_searcher() const {
                mem_bound_t states = get_content();
                size_t states_bytes = states.tail - states.data;
                Searcher* ptr = (Searcher*)malloc(sizeof(Searcher) + states_bytes);
                if (!ptr) return ptr;
                ptr->scount = states_.size();
                ptr->flags = options_;
                memcpy(ptr->states, states.data, states_bytes);
                return ptr;
            }

            const mem_bound_t get_content() const {
                mem_bound_t mb;
                mb.data = (uint8_t*)root_;
                mb.tail = mb.data + states_.size() * sizeof(state_t);
                return mb;
            }
            bool build(keywords_t & keywords, uint8_t options) {
                options_ = options;
                typename keyword_t::less_compare_t less((options & ACFF_CASE_SENSITIVE)!= 0);
                std::sort(keywords.begin(), keywords.end(), less);
                size_t depth = alloc_states_(keywords);
                level_info_t level1(keywords.size());
                level_info_t level2(keywords.size());
                level_info_t * lev_curr = &level1, *lev_next = &level2;
                for (size_t i = 0; i < depth + 1; ++i) {
                    if (!build_level(keywords, i, lev_curr, lev_next))
                        return false;
                    std::swap(lev_curr, lev_next);
                    lev_next->reset();
                }
                states_.resize(alloced_);
                return true;
            }
        };

    }

}