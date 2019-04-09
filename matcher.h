#pragma once
#include <stdint.h>
#include <deque>
#include <list>
#include <vector>
#include <stack>

namespace mp {

    enum {
        bc_plain,
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

        struct bc_bound_t {
            uint16_t head;
            uint16_t tail;
        };

        typedef union {
            struct {
                const char* ptr;
                const char* tail;
            } plain_;

            struct {
                uint32_t   rest_;
            } jump_;

            // 10s
            struct {
                uint16_t    min_;
                uint16_t    max_;
                uint16_t    cnt_;
            } repeat_;

            struct {
                uint32_t    rest_;
                uint32_t    state_;
                sms_1_t* table_;         // 8
            } search_1_;

            struct {
                uint32_t    rest_;
                uint32_t    state_;
                sms_n_t* table_;
            } search_n_;

            struct {
                uint32_t    rest_;
                uint32_t    state_;
                smm_n_t* table_;
            } match_n_;

            struct {
                uint16_t    base_;
                uint16_t    branches_;
            } either_;

        } un_state_t;

        // 1 + 1 + 2 + 2 + 4 + 12 = 22
        struct match_cell_t {
            uint8_t    kind;
            uint8_t    flags;
            un_state_t state;       // 当前的匹配的状态(临时变量)
        };

        struct mem_bound_t {
        };

        using _instr_new_t      = bool(*)(match_cell_t&);
        using _instr_del_t      = void(*)(match_cell_t&);
        using _instr_match_t    = int(*)(match_cell_t&, mem_bound_t&);

        struct instr_t {
            _instr_new_t    new_;
            _instr_del_t    del_;
            _instr_match_t  match_;
        };

        static const instr_t INSTR_TABLE[0x100] = {
            {},
            {},
            {},
            {},
            {},
            {},
            {},
            {},
            {},
            {},
            {},
        };

        struct program_t {

        };

        enum {
            MR_ERROR = -1,
            MR_CONTINUE = 0,
            MR_UNMATCH = 1,
            MR_MATCHED = 2,
        };
        enum {
            CELL_DROPPED = 1,
            FLAG_CS = 2,
        };
        struct pattern_matcher_t {
            typedef std::deque<match_cell_t> stack_t;
            stack_t     nodes_;
            program_t   program_;
            int match(uint8_t ch) {
                return _do_cell(nodes_[0]);
            }
            enum {
                kPlain,
                kEither,
            };
            inline int _do_cell(match_cell_t& cell, uint8_t ch) {
                if (cell.flags & CELL_DROPPED)
                    return MR_CONTINUE;
                switch (cell.kind) {
                case kPlain:
                    _do_plain();
                }
            }
            void node_remove_top(size_t last) {
                if (last == nodes_.size()) {
                    while (nodes_.back().flags & CELL_DROPPED) {
                        nodes_.pop_back();
                    }
                }
            }
            int _new_either(match_cell_t& either) {
                uint8_t branches = 0;
                if (!program_.fetch(branches))
                    return MR_ERROR;

                either.kind = kEither;
                size_t base = either.state.either_.base = nodes_.alloc(branches);
                size_t len = either.state.either_.branches = branches;

                for (size_t i = 0; i < branches; ++i) {
                    uint8_t len = 0;
                    if (!program_.fetch(len))
                        return MR_ERROR;
                    _init_cell(nodes_[base], program_.pc(), len);
                    program_.seek(len);
                }
            }
            int _do_either(match_cell_t& cell, uint8_t ch) {
                size_t base = cell.state.either_.base_;
                size_t branches = cell.state.either_.branches_;
                for (size_t i = 0; i < branches; ++i) {
                    size_t child = base + i;
                    auto& node = nodes_[i];
                    int r = _do_cell(node, ch);
                    if (r == MR_MATCHED)
                        return MR_MATCHED;
                    else if (r == MR_UNMATCH) {
                        node.flags |= CELL_DROPPED;
                    }
                }
                return MR_CONTINUE;
            }
            void _del_either(match_cell_t& cell) {
                // 
                size_t base = cell.state.either_.base_;
                size_t branches = cell.state.either_.branches_;
                size_t last = base;
                for (; last < base + branches; ++last) {
                    nodes_[last].flags |= CELL_DROPPED;
                }
                node_remove_top(last);
            }

            int _init_repeat(match_cell_t& repeat) {
                uint8_t min_ = 0, max_ = 0, len = 0;
                if (!program_.fetch(min_) ||
                    !program_.fetch(max_) ||
                    !program_.fetch(len_))
                {

                }
            };
            int _do_plain(match_cell_t& cell, uint8_t ch) {
                uint8_t pch;
                if (!program_.fetch(cell.bound, pch))
                    return -1;
                return (cell.flags & FLAG_CS) ?
                    pch == ch : ::towlower(pch) == ::towlower(ch);
            }
            int _do_skip(match_cell_t& cell, uint8_t ch) {
                const size_t n = --cell.state.jump_.rest;
                return n == 0;
            }
            int _do_repeat(match_cell_t& cell, uint8_t ch) {
                const size_t n = --cell.state.jump_.rest;
                if (!n) {

                }
            }
        };

        struct prefix_searcher_t {

        };

        enum {
            PTN_DUP_HIT = 1,
        };

        struct lib_pattern_t {
            uint8_t duphit : 1;
            uint8_t icase : 1;
            uint8_t pures : 1;
            uint8_t len;
            int16_t score;
        };

        struct lib_rule_t {
            uint32_t rule_id;
            uint16_t score;
            uint8_t has_minus : 1;
            uint8_t ptn_cnt;
        };

        template < typename T >
        struct const_arry_t {
            size_t size;
            const T* elements;
            const T& operator [] (size_t i) const {
                return elements[i];
            }
        };

        class score_board_t {
        protected:
            std::vector<int>    scores_;
        public:
            bool check_score(size_t index, const lib_rule_t* rule, int score)
            {
                int result = (scores_[index] += score);
                if (rule->has_minus) return false;
                if (rule->score <= result) return true;
                return false;
            }
        };

        enum
        {
            OPTR = 1,
            ALPH = 2,
            UPCS = 4,
            ALPU = ALPH | UPCS,
            NUMB = 8,
            EDKC = 16,
            ALPL = ALPH,
            HEXC = NUMB,
            ALOP = OPTR | ALPH,
        };

        const uint8_t CHAMAP[256] = {
            0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      ,
            0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      ,
            0x00      , OPTR      , OPTR      , OPTR      , OPTR      , EDKC | OPTR , EDKC | OPTR , OPTR      , OPTR      , OPTR      , OPTR      , OPTR      , OPTR      , OPTR      , ALOP      , OPTR      ,
            NUMB      , NUMB      , NUMB      , NUMB      , NUMB      , NUMB      , NUMB      , NUMB      , NUMB      , NUMB      , OPTR      , OPTR      , OPTR      , OPTR      , OPTR      , OPTR      ,
            OPTR      , ALPU | HEXC , ALPU | HEXC , ALPU | HEXC , ALPU | HEXC , ALPU | HEXC , ALPU | HEXC , ALPU      , ALPU      , ALPU      , ALPU      , ALPU      , ALPU      , ALPU      , ALPU      , ALPU      ,
            ALPU      , ALPU      , ALPU      , ALPU      , ALPU      , ALPU      , ALPU      , ALPU      , ALPU      , ALPU      , ALPU      , OPTR      , EDKC | OPTR , OPTR      , OPTR      , OPTR      ,
            OPTR      , ALPL | HEXC , ALPL | HEXC , ALPL | HEXC , ALPL | HEXC , ALPL | HEXC , ALPL | HEXC , ALPL      , ALPL      , ALPL      , ALPL      , ALPL      , ALPL      , ALPL      , ALPL      , ALPL      ,
            ALPL      , ALPL      , ALPL      , ALPL      , ALPL      , ALPL | EDKC , ALPL      , ALPL      , ALPL | EDKC , ALPL      , ALPL      , OPTR      , OPTR      , OPTR      , OPTR      , OPTR      ,
            0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      ,
            0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      ,
            0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      ,
            0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      ,
            0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      ,
            0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      ,
            0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      ,
            0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00      , 0x00
        };


        class rule_database_t {
        public:
            const_arry_t<lib_rule_t*> rules_;
            const_arry_t<lib_pattern_t*> patterns_;
            size_t max_plain_;
        };

        struct escape_x_t {
            uint8_t size;
            uint8_t save[4];
        };
        struct escape_u_t {
            uint8_t size;
            uint8_t save[6];
        };
        struct escape_o_t {
            uint8_t size;
            uint8_t save[4];
            // 
            size_t feed(uint8_t ch, uint8_t attr) {
                save[size++] = ch;
                if (attr & OCT) {
                    if (size == sizeof(save)) {
                        uint8_t ch =
                            (save[1] - '0') << 6 |
                            (save[2] - '0') << 3 |
                            (save[3] - '0');
                        save[0] = ch;
                        return 1;
                    }
                    return 0;
                }
                else {
                    return size;
                }
            }
            const uint8_t* data() {
                return save;
            }
        };

        enum {
            _normal_mode,
            _escape_x_mode,
            _escape_u_mode,
            _escape_o_mode,
            _unescape_x_mode,
            _unescape_u_mode,
        };
        struct any_state_t {
            uint8_t state;
            size_t  depth;
            uint8_t save[16];
            uint8_t size;
            
            size_t feed(const uint8_t& ch, uint8_t attr) {
                save[size++] = ch;

                switch (state) {
                case _normal:
                    if (size == 2) {
                        if (save[0] == '\\' && save[1] == 'x') {
                            state = _escape_x_mode;
                        }
                        else if (save[0] == '\\' && save[1] == 'u') {
                            state = _escape_u_mode;
                        }
                        else if (save[0] == '\\' && '0' <= save[1] && save[1] <= '8') {
                            state = _escape_o_mode;
                        }
                        if (save[0] == '%' && save[1] == 'x') {
                            state = _unescape_x_mode;
                        }
                        if (save[0] == '%' && save[1] == 'u') {
                            state = _unescape_u_mode;
                        }
                        else {
                            size = 0;
                            return 2; 
                        }
                    }
                    break;
                case _escape_x_mode:
                    if( !is_hex(ch) ) {
                        // 坏字符
                        return 
                    } else {
                        -- save;

                    }
                }
            };
            enum status_t {
                _normal,
                _space,
                _escape_mode,		// '\'
                _unescape_mode,		// '%'
                _html_escape_mode,
                _hex_8_mode,		// 
                _hex_16_mode,
                _dec_html_mode,
                _oct_mode,
            };
            class nest_decoder_t {
            public:
                std::deque<any_state_t> states_;
            public:
                size_t push_pop(uint8_t ch) {
                    switch (states_[0].state) {

                    }
                }
            };

            class decoder_t {
            protected:
                // \x, \u, %
                enum status_t {
                    _normal,
                    _space,
                    _escape_mode,		// '\'
                    _unescape_mode,		// '%'
                    _html_escape_mode,
                    _hex_8_mode,		// 
                    _hex_16_mode,
                    _dec_html_mode,
                    _oct_mode,
                };
                size_t dec_num = 0;
                uint8_t saved[16] = {};
                size_t  savec = 0;
                size_t  trans_start = 0;
                status_t ss = _normal;
            public:
                static size_t try_decode(status_t ss, uint8_t* text, size_t bytes, size_t* changed)
                {
                    const uint8_t* scan = text;
                    const uint8_t* tail = scan + bytes;
                    uint8_t* wp = text;
                    uint8_t attr_before_space = 0;
                    for (; scan < tail + 1; ++scan)
                    {
                        uint8_t c = scan < tail ? *scan : 0;
                        uint8_t attr = CHAMAP[c];
                        if (_space == ss)
                        {
                            if (!attr) continue;
                            ss = _normal;
                            --scan;
                            continue;
                        }
                        if (_normal == ss)
                        {
                            if (!attr) {
                                ss = _space;
                                continue;
                            }
                            else {
                                attr_before_space = attr;
                            }
                            if (c == '\\')
                            {
                                saved[savec++] = c;
                                ss = _escape_mode;
                            }
                            else if (c == '%')
                            {
                                saved[savec++] = c;
                                ss = _unescape_mode;
                            }
                            else
                            {
                                if (c == '\"') c = '\'';
                                *wp = c; ++wp;
                            }
                            continue;
                        }

                        saved[savec++] = c;
                        if (_dec_html_mode == ss)
                        {
                            if (attr == NUMB) {
                                continue;
                            }
                            else if (c == ';') {
                                // dec end
                                size_t bc = dec_to_i(saved + trans_start, savec - trans_start - 1, wp);
                                if (bc)
                                {
                                    wp += bc;
                                    savec = 0;
                                }
                            }
                        }
                        else if (_oct_mode == ss)
                        {
                            if (attr == NUMB && c < '8' && savec - trans_start < 4) {
                                continue;
                            }
                            else if (oct_to_i(saved + trans_start, savec - trans_start - 1, wp))
                            {
                                ++wp;
                                dec_num++;
                                savec = 0;
                                --scan;
                            }
                        }
                        else if (_escape_mode == ss)
                        {
                            if (c == 'x')
                            {
                                trans_start = savec;
                                ss = _hex_8_mode;
                                continue;
                            }
                            if (c == 'u')
                            {
                                trans_start = savec;
                                ss = _hex_16_mode;
                                continue;
                            }
                            if (attr == NUMB && c < '8')
                            {
                                trans_start = savec - 1;
                                ss = _oct_mode;
                                continue;
                            }
                        }
                        else if (_unescape_mode == ss)
                        {
                            if (c == 'u')
                            {
                                trans_start = savec;
                                ss = _hex_16_mode;
                                continue;
                            }
                            if (CHAMAP[c] & HEXC)
                            {
                                trans_start = savec - 1;
                                ss = _hex_8_mode;
                                continue;
                            }
                        }
                        else if (_hex_8_mode == ss)
                        {
                            if (CHAMAP[c] & HEXC)
                            {
                                if (savec - trans_start < 2) continue;
                                hex2_2_i(saved + trans_start, wp); ++wp;
                                dec_num++;
                                savec = 0;
                            }
                        }
                        else if (_hex_16_mode == ss)
                        {
                            if (CHAMAP[c] & HEXC)
                            {
                                if (savec - trans_start < 4) continue;
                                hex4_2_i(saved + trans_start, wp); wp += 2;
                                dec_num++;
                                savec = 0;
                            }
                        }
                        // if savec > 0, means has bad char, need re-process it
                        if (savec)
                        {
                            savec--;
                            if (savec) { memcpy(wp, saved, savec); wp += savec; }
                            --scan;		// re-process in normal mode
                            savec = 0;
                        }
                        ss = _normal;
                    }
                    if (savec)
                    {
                        memcpy(wp, saved, savec);
                        wp += savec;
                    }
                    *changed = dec_num;
                    return wp - text;
                }
            };


            struct script_normalizer_t {
                void execute(plain_scan_t& handler, mem_bound_t& mb) {
                    uint8_t normal[16] = {};
                    uint8_t alnum[16] = {};
                    uint8_t optor[16] = {};

                    uint8_t b = 0;
                    for (; mb.fetch(b); ) {
                        size_t k = CHAMAP[b];
                        if (k & UPCS) b = b + 0x20;
                        normal[li++] = b;
                    }
                    for (size_t li = 0;; mb.rest(); li = 0) {
                        for (; li < sizeof(local); ) {
                            uint8_t b = 0;
                            if (!mb.fetch(b))
                                break;
                            size_t k = CHAMAP[b];
                            if (k & UPCS) b = b + 0x20;
                            local[li++] = b;
                        }
                        if (li) {
                            handler.match(local, li);
                            li = 0;
                        }
                    }
                }
                void execute(plain_scan_t& handler, mem_bound_t& mb) {
                    uint8_t local[16] = {};
                    for (size_t li = 0;; mb.rest(); li = 0) {
                        for (; li < sizeof(local); ) {
                            uint8_t b = 0;
                            if (!mb.fetch(b))
                                break;
                            size_t k = CHAMAP[b];
                            if (k & UPCS) b = b + 0x20;
                            local[li++] = b;
                        }
                        if (li) {
                            handler.match(local, li);
                            li = 0;
                        }
                    }
                }
            };


            struct script_alnumlizer_t {
                void execute(plain_scan_t& handler, mem_bound_t& mb) {
                    for (size_t i = 0; i <
                }
            };
                        struct script_skeletonlizer_t {
                void execute(plain_scan_t& handler, mem_bound_t& mb) {

                }
            };
                // stuff, normal, alnum, skeleton
                struct script_file_scan_t {
                enum {
                    PLAIN_STUFF,
                    PLAIN_NORMAL,
                    PLAIN_ALNUM,
                    PLAIN_SKELETON,
                    PLAIN_MAX
                };
                plain_scan_t plain_scan[PLAIN_MAX];
                script_normalizer_t normal_;
                script_alnumlizer_t alnum_;
                script_skeletonlizer_t skeleton_;
                bool execute(mem_bound_t& mb) {
                    mem_bound_t scan = mb;
                    if (plain_scan[PLAIN_STUFF].scan(scan))
                        return true;
                    scan = mb;
                    if (normal_.execute(plain_scan[PLAIN_NORMAL], scan))
                        return true;
                    scan = mb;
                    if (alnum_.execute(plain_scan[PLAIN_ALNUM], scan))
                        return true;
                    scan = mb;
                    if (skeleton_.execute(plain_scan[PLAIN_NORMAL], scan))
                        return true;
                    return false;
                }
            };

            struct plain_scan_t {
            public:
                std::list<pattern_matcher_t> patterns_;
                prefix_searcher_t prefix_search_;
                size_t prefix_search_state_;
                mem_bound_t lib_patterns_*;
                score_board_t score_board_;
            public:
                void on_pattern_hit(pattern_matcher_t& pattern) {
                    int score = pattern.score();
                    uint32_t rule_id = pattern.rule_id();
                    int mr = score_board_.check_score(rule_id, score);
                    if (mr) {
                        // alarm(rule_id);
                    }
                }
                int scan(uint8_t d) {
                    for (auto& pattern : patterns_) {
                        mem_bound_t d = data;
                        int mr = pattern.match(d);
                        if (mr == MR_CONTINUE) {
                            continue;
                        }
                        else if (mr == MR_MATCHED) {
                            on_pattern_hit(pattern);
                            if (pattern.flags & PTN_DUP_HIT)
                                continue;
                        }
                        // UNMATCH or ERROR
                        // remove pattern from list
                    }
                    mem_bound_t d = data;
                    std::list<pattern_matcher_t> incr_patterns;
                    while (d.size()) {
                        if (prefix_search_.match(prefix_search_state_, d)) {
                            size_t k = prefix_search_;
                            uint32_t user = 0;
                            while (prefix_search_.get_final(k, user)) {
                                pattern_matcher_t new_ptn(lib_patterns_[user]);
                                new_ptn.init();
                                mem_bound_t dd = data;
                                int mr = new_ptn.match(dd);
                                if (mr == MR_CONTINUE) {
                                    incr_patterns.emplace_back(new_ptn);
                                    continue;
                                }
                                else if (mr == MR_MATCHED) {
                                    on_pattern_hit(new_ptn);
                                    if (new_ptn.flags & PTN_DUP_HIT) {
                                        incr_patterns.emplace_back(new_ptn);
                                        continue;
                                    }
                                }
                                new_ptn.match(d);
                            }
                        }
                    }
                    patterns_.splice(patterns_.end(), incr_patterns);
                }
            }
            int scan(mem_bound_t & data) {
                for (auto& pattern : patterns_) {
                    mem_bound_t d = data;
                    int mr = pattern.match(d);
                    if (mr == MR_CONTINUE) {
                        continue;
                    }
                    else if (mr == MR_MATCHED) {
                        on_pattern_hit(pattern);
                        if (pattern.flags & PTN_DUP_HIT)
                            continue;
                    }
                    // UNMATCH or ERROR
                    // remove pattern from list
                }
                mem_bound_t d = data;
                std::list<pattern_matcher_t> incr_patterns;
                while (d.size()) {
                    if (prefix_search_.match(prefix_search_state_, d)) {
                        size_t k = prefix_search_;
                        uint32_t user = 0;
                        while (prefix_search_.get_final(k, user)) {
                            pattern_matcher_t new_ptn(lib_patterns_[user]);
                            new_ptn.init();
                            mem_bound_t dd = data;
                            int mr = new_ptn.match(dd);
                            if (mr == MR_CONTINUE) {
                                incr_patterns.emplace_back(new_ptn);
                                continue;
                            }
                            else if (mr == MR_MATCHED) {
                                on_pattern_hit(new_ptn);
                                if (new_ptn.flags & PTN_DUP_HIT) {
                                    incr_patterns.emplace_back(new_ptn);
                                    continue;
                                }
                            }
                            new_ptn.match(d);
                        }
                    }
                }
                patterns_.splice(patterns_.end(), incr_patterns);
            }
        };


    };


};