#pragma once
#include "sm_descape.h"

namespace mp {

    using namespace ac;

    template < size_t MAX_BYTE_QUEUE_SIZE = 64>
    class normalize_t : public NestUnescape< MAX_BYTE_QUEUE_SIZE, 16>
    {
    };

    template < size_t MAX_BYTE_QUEUE_SIZE, size_t TrairMask>
    class char_trait_filter_t {
    protected:
        uint8_t bytes_[MAX_BYTE_QUEUE_SIZE];
        uint16_t size_ = 0;
    public:
        static const size_t TraitsToSelect = TrairMask;
        size_t forward(mem_bound_t& mb) {
            const uint8_t* scan = mb.data;
            for (; scan < mb.tail; ++scan) {
                uint8_t ch = *scan;
                uint8_t tr = CharTrait::Table[ch];
                if (tr & TraitsToSelect) {
                    bytes_[size_++] = ch;
                    if (size_ == MAX_BYTE_QUEUE_SIZE) {
                        size_ = 0;
                        mb.data = scan + 1;
                        return MAX_BYTE_QUEUE_SIZE;
                    }
                }
            }
            mb.data = scan;
            return 0;
        }
        inline size_t forward(uint8_t ch) {
            uint8_t tr = CharTrait::Table[ch];
            if (tr & (CharTrait::Alpha | CharTrait::DecChar)) {
                bytes_[size_++] = ch;
                if (size_ == MAX_BYTE_QUEUE_SIZE) {
                    size_ = 0;
                    return MAX_BYTE_QUEUE_SIZE;
                }
            }
            return 0;
        }
        inline size_t finsh() {
            return size_;
        }
        inline const void* data() {
            return bytes_;
        }
    };

    template < size_t MAX_BYTE_QUEUE_SIZE = 64>
    class alphanum_t :
        public char_trait_filter_t<
        MAX_BYTE_QUEUE_SIZE,
        CharTrait::Alpha | CharTrait::DecChar
        >
    {
    };

    template < size_t MAX_BYTE_QUEUE_SIZE = 64>
    class operator_t :
        public char_trait_filter_t<
        MAX_BYTE_QUEUE_SIZE,
        CharTrait::OPTR
        >
    {
    };

    template <typename Machine >
    class forward_match_t {
    protected:
        const Machine& machine_;
        typename Machine::MatchStateId state_ = 0;
    public:
        forward_match_t(Machine& machine) : machine_(machine) {
        }
        void forward(mem_bound_t& mb) {
            ac::match(machine_, state_, mb, false);
        }
        void finsh() {

        }
    };

    template < typename FROM, typename TO>
    class forward_chain_t {
    protected:
        FROM& from_ = 0;
        TO& to_ = 0;
        size_t intput_ = 0, output_ = 0;
    public:
        forward_chain_t(FROM& from, TO& to) : from_(from), to_(to) {
        }
        void forward(mem_bound_t& mb)
        {
            intput_ += mem_bound_size(mb);
            while (mem_bound_size(mb)) {
                size_t ready = from_.forward(mb);
                if (!ready) continue;
                output_ += ready;
                mem_bound_t out;
                mem_bound_init(out, from_.data(), ready);
                while (mem_bound_size(out))
                    to_.forward(out);
            }
        }
        void finsh() {
            size_t ready = from_.finsh();
            output_ += ready;
            if (ready) {
                mem_bound_t out;
                mem_bound_init(out, from_.data(), ready);
                while (mem_bound_size(out))
                    to_.forward(out);
            }
            to_.finsh();
        }
    };

    template <typename T >
    void speed_test(T& forwarder, const std::string& text, size_t loop) {
        const char* class_name = typeid(T).name();
        double total = text.size() * loop;
        ac::mem_bound_t in;
        size_t start = GetTickCount();
        for (size_t k = 0; k < loop; ++k) {
            ac::mem_bound_init(in, text);
            while (ac::mem_bound_size(in)) {
                forwarder.forward(in);
            }
        }
        double durtion = GetTickCount() - start;
        printf("%s, durtion: %zd ms, forward: %0.2f MB, speed; %0.2f Mb/s\n",
            class_name, (size_t)durtion, total / 1024 / 1024, total / durtion * 1000 / 1024 / 1024 * 8);
    }

};