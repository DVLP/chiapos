// Copyright 2018 Chia Network Inc

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef SRC_CPP_FAST_SORT_ON_DISK_HPP_
#define SRC_CPP_FAST_SORT_ON_DISK_HPP_

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "chia_filesystem.hpp"

#include "./bits.hpp"
#include "./calculate_bucket.hpp"
#include "./disk.hpp"
#include "./quicksort.hpp"
#include "./uniformsort.hpp"
#include "disk.hpp"
#include "exceptions.hpp"

#include "entry_sizes.hpp"

enum class strategy_t : uint8_t
{
    uniform,
    quicksort,

    // the quicksort_last strategy is important because uniform sort performs
    // really poorly on data that isn't actually uniformly distributed. The last
    // buckets are often not uniformly distributed.
    quicksort_last,
};

class SortManager : public Disk {
public:
    SortManager(
        uint64_t const param_memory_size,
        uint32_t const num_buckets,
        uint32_t const log_num_buckets,
        uint16_t const entry_size,
        const std::string &tmp_dirname,
        const std::string &filename,
        uint32_t begin_bits,
        uint64_t const stripe_size,
        uint64_t const entries_per_bucket,
        strategy_t const sort_strategy = strategy_t::uniform)
        : entry_size_(entry_size)
        , begin_bits_(begin_bits)
        , log_num_buckets_(log_num_buckets)
        , prev_bucket_buf_size(
            2 * (stripe_size + 10 * (kBC / pow(2, kExtraBits))) * entry_size)
        // , memory_per_bucket_(param_memory_size / num_buckets)
        // 7 bytes head-room for SliceInt64FromBytes()
        , entry_buf_(new uint8_t[entry_size + 7])
        , strategy_(sort_strategy)
    {
        max_bucket_entries_ = entries_per_bucket;

        // if (entries_per_bucket < 2 * (stripe_size + 10 * (kBC / pow(2, kExtraBits)))) {
        //     std::cout << "Error: entries_per_bucket not enough: " << entries_per_bucket << " / " << 2 * (stripe_size + 10 * (kBC / pow(2, kExtraBits))) << std::endl;
        //     exit(1);
        // } else if (entries_per_bucket > 2 * (stripe_size + 10 * (kBC / pow(2, kExtraBits))) + 2000) {
        //     std::cout << "Error: memory size is not enough, or not enough buckets";
        //     exit(1);
        // }

        std::cout << "Max bucket entries " << max_bucket_entries_ << std::endl;
        std::cout << "MAX BUCKET ENTRIES: " << max_bucket_entries_ << std::endl;
        // memory_per_bucket_ = prev_bucket_buf_size;
        memory_per_bucket_ = max_bucket_entries_ * entry_size;
        memory_per_bucket_ = memory_per_bucket_ > 1024 * 1024 ? memory_per_bucket_ : (1024 * 1024);
        memory_size_ = memory_per_bucket_ * num_buckets;
        std::cout << "SortManager" << std::endl;
        std::cout << "Num buckets: " << num_buckets << std::endl;
        std::cout << "Memory size: " << memory_size_ << " B   " << (memory_size_ / 1024 / 1024) << "MB" << std::endl;
        std::cout << "Memory per bucket: " << memory_per_bucket_ << " B   " << (memory_per_bucket_ / 1024 / 1024) << "MB" << std::endl;
// 26 - 512
// 27 - 1024
// 28 - 2048
// 29 - 4046
// 30 - 8092
// 31 - 16384
// 32 - 32768
        // this is only valid for certain settings
        // if (memory_per_bucket_ < 298254336) {
        //     std::cout << "Not enough memory for per-bucket allocation. Increase -b to minimum): " << (298254336 * num_buckets / 1024) << " MB" << std::endl;
        // }
        // assert(memory_per_bucket_ <= 298254336);
        if (!memory_start_) {
            // preallocation only needed for
            // in FreeMemory() or the destructor
            std::cout << "restart memory: " << memory_size_ << std::endl;
            memory_start_.reset();
            memory_start_.reset(new uint8_t[memory_size_]);
            std::cout << "memory restarted" << std::endl;
        }

        // Cross platform way to concatenate paths, gulrak library.
        std::vector<fs::path> bucket_filenames = std::vector<fs::path>();

        buckets_.reserve(num_buckets);
        for (size_t bucket_i = 0; bucket_i < num_buckets; bucket_i++) {
            std::vector<std::array<uint8_t, 7>> bucket_buffer;
            buckets_.emplace_back(
                bucket_buffer);
        }
    }

    void AddToCache(const Bits &entry)
    {
        entry.ToBytes(entry_buf_.get());
        return AddToCache(entry_buf_.get());
    }

    uint8_t *GetNextCachePos(const uint8_t *entry)
    {
        if (this->done) {
            throw InvalidValueException("Already finished.");
        }
        uint64_t const bucket_index =
            Util::ExtractNum(entry, entry_size_, begin_bits_, log_num_buckets_);
        bucket_t& b = buckets_[bucket_index];

        if (b.num_entries + 1 > max_bucket_entries_) {
            std::cout << "Max bucket entries exceeded: " << b.num_entries << " / " << max_bucket_entries_ << std::endl;
            throw InvalidStateException("AddToCache: Max bucket entries exceeded");
        }

        // if (b.num_entries * entry_size_ + entry_size_ > memory_per_bucket_) {
        //     std::cout << "Not enough memory in this bucket!" << std::endl;
        //     std::cout << "Num entries: " << b.num_entries << std::endl;
        //     std::cout << "Req memory MB(per bucket): " << (b.num_entries * entry_size_ / 1024 / 1024) << std::endl;
        //     std::cout << "Req memory MB(total): " << (log_num_buckets_ * log_num_buckets_ * b.num_entries * entry_size_ / 1024 / 1024) << std::endl;
        //     throw InvalidStateException("AddToCache: Per bucket out of memory bounds write attempt");
        // }

        // if ((bucket_index * memory_per_bucket_) + b.num_entries * entry_size_ + entry_size_ > memory_size_) {
        //     std::cout << "Not enough memory in total" << std::endl;
        //     std::cout << "Num entries: " << b.num_entries << std::endl;
        //     std::cout << "Req memory MB: " << (b.num_entries * entry_size_ / 1024 / 1024) << std::endl;
        //     throw InvalidStateException("AddToCache: Total out of memory bounds write attempt");
        // }
        // ::memcpy(memory_start_.get() + (bucket_index * memory_per_bucket_) + (b.num_entries * entry_size_), entry, entry_size_);

        b.write_pointer += entry_size_;
        b.num_entries++;

        return memory_start_.get() + (bucket_index * memory_per_bucket_) + (b.num_entries * entry_size_);

        // assert(b.num_entries * entry_size_ == b.write_pointer);
    }

    void AddToCache(const uint8_t *entry)
    {
        // if (this->done) {
        //     throw InvalidValueException("Already finished.");
        // }
        uint64_t const bucket_index =
            Util::ExtractNum(entry, entry_size_, begin_bits_, log_num_buckets_);
        bucket_t& b = buckets_[bucket_index];

        if (b.num_entries + 1 > max_bucket_entries_) {
            std::cout << "Max bucket entries exceeded: " << b.num_entries << " / " << max_bucket_entries_ << std::endl;
            throw InvalidStateException("AddToCache: Max bucket entries exceeded");
        }

        // if (b.num_entries * entry_size_ + entry_size_ > memory_per_bucket_) {
        //     std::cout << "Not enough memory in this bucket!" << std::endl;
        //     std::cout << "Num entries: " << b.num_entries << std::endl;
        //     std::cout << "Req memory MB(per bucket): " << (b.num_entries * entry_size_ / 1024 / 1024) << std::endl;
        //     std::cout << "Req memory MB(total): " << (log_num_buckets_ * log_num_buckets_ * b.num_entries * entry_size_ / 1024 / 1024) << std::endl;
        //     throw InvalidStateException("AddToCache: Per bucket out of memory bounds write attempt");
        // }

        // if ((bucket_index * memory_per_bucket_) + b.num_entries * entry_size_ + entry_size_ > memory_size_) {
        //     std::cout << "Not enough memory in total" << std::endl;
        //     std::cout << "Num entries: " << b.num_entries << std::endl;
        //     std::cout << "Req memory MB: " << (b.num_entries * entry_size_ / 1024 / 1024) << std::endl;
        //     throw InvalidStateException("AddToCache: Total out of memory bounds write attempt");
        // }
        ::memcpy(memory_start_.get() + (bucket_index * memory_per_bucket_) + (b.num_entries * entry_size_), entry, entry_size_);

        b.write_pointer += entry_size_;
        b.num_entries++;

        // assert(b.num_entries * entry_size_ == b.write_pointer);
    }

    uint8_t const* Read(uint64_t begin, uint64_t length) override
    {
        assert(length <= entry_size_);
        return ReadEntry(begin);
    }

    void Write(uint64_t, uint8_t const*, uint64_t) override
    {
        assert(false);
        throw InvalidStateException("Invalid Write() called on SortManager");
    }

    void Truncate(uint64_t new_size) override
    {
        if (new_size != 0) {
            assert(false);
            throw InvalidStateException("Invalid Truncate() called on SortManager");
        }

        FlushCache();
        FreeMemory();

        // TODO: reset num_entries this on truncate?
    }

    std::string GetFileName() override
    {
        return "<SortManager>";
    }

    void FreeMemory() override
    {
        // TODO: get rid of these FreeMemory and FlushCache function as files are already removed and the object can be destroyed as well
        // for (auto& b : buckets_) {
            // TODO: it causes "Trying to sort bucket which does not exist" in phase 2
            // b.write_pointer = 0;
            // b.num_entries = 0;
        // }

        // memory_start_.reset(); // do not reset memory, it's not just one bucket cache anymore but all of them
        // final_position_end = 0;
    }

    uint8_t *ReadEntry(uint64_t position)
    {
        if (position < this->final_position_start) {
            if (!(position >= this->prev_bucket_position_start)) {
                throw InvalidStateException("Invalid prev bucket start");
            }

            return memory_start_.get() + this->prev_bucket_position_offset + position - this->prev_bucket_position_start;
        }

        while (position >= this->final_position_end) {
            SortBucket();
        }
        if (!(this->final_position_end > position)) {
            throw InvalidValueException("Position too large");
        }
        if (!(this->final_position_start <= position)) {
            throw InvalidValueException("Position too small");
        }
        assert(memory_start_);

        return memory_start_.get() + (memory_per_bucket_ * (this->next_bucket_to_sort - 1)) + (position - this->final_position_start);
    }

    bool CloseToNewBucket(uint64_t position) const
    {
        if (!(position <= this->final_position_end)) {
            return this->next_bucket_to_sort < buckets_.size();
        };
        return (
            position + prev_bucket_buf_size / 2 >= this->final_position_end &&
            this->next_bucket_to_sort < buckets_.size());
    }

    void TriggerNewBucket(uint64_t position)
    {
        if (!(position <= this->final_position_end)) {
            throw InvalidValueException("Triggering bucket too late");
        }
        if (!(position >= this->final_position_start)) {
            throw InvalidValueException("Triggering bucket too early");
        }

        if (memory_start_) {
            // save some of the current bucket, to allow some reverse-tracking
            // in the reading pattern,
            // position is the first position that we need in the new array
            prev_bucket_index_ = this->next_bucket_to_sort - 1;
            uint64_t const bucket_offset = memory_per_bucket_ * prev_bucket_index_;
            this->prev_bucket_position_offset = bucket_offset + position - this->final_position_start;
        }

        SortBucket();
        this->prev_bucket_position_start = position;
    }

    void FlushCache()
    {
        // for (auto& b : buckets_) {
        //     // TODO: Actually FlushCache only ever writes bucket files, not tables. Writing tables happens at the end of phases
        //     // TODO: flush does not mean clear but i.e. serialize data to drive
        // }

        // final_position_end = 0;
        // memory_start_.reset(); // do not flush in single block
    }

    ~SortManager()
    {
        // Close and delete files in case we exit without doing the sort
        for (auto& b : buckets_) {
            // TODO: does this solve the issue with incorrect number of entries?
            b.write_pointer = 0;
            b.num_entries = 0;
        }
        memory_start_.reset();
    }

private:

    struct bucket_t
    {
        std::vector<std::array<uint8_t, 7>> buff;

        bucket_t(std::vector<std::array<uint8_t, 7>> buf) : buff(buf) {
        }

        uint64_t num_entries = 0;

        // The amount of data written to the disk bucket
        uint64_t write_pointer = 0;
        
        // std::future<void> future;
        // uint8_t started = 0;
        // std::unique_ptr<uint8_t[]> buffer_;
    };

    // The buffer we use to sort buckets in-memory
    std::unique_ptr<uint8_t[]> memory_start_;
    std::unique_ptr<uint8_t[]> sorting_memory_;
    std::bitset<500000000UL> used_bitset;

// 21 = 125k + 0.5 -> 187k max   0.5m bitset
// 22 = 0.25m + 0.5 -> 0.37m   3.5m
// 23 = 0.5m + 0.5 -> 0.75m   1.7m
// 24 = 1m + 0.5 -> 1.5m   3.5m
// 25 = 2m + 0.5 ->3m      7m
// 26 = 4 + 0.5 -> 6m      15m
// 27 = 8m + 0.5 -> 12m    31m
// 28 = 16m + 0.5 -> 24m   62.5m
// 29 = 32m + 0.5 -> 48m   125m
// 30 = 64m + 0.5 -> 96m   250m
// 31 = 128m + 0.5 -> 192m 500m
// 32 = 256m + 0.5 -> 374m 1B

    uint64_t max_sorting_memory_;
    // Size of the whole memory array
    uint64_t memory_size_;

    uint64_t memory_per_bucket_;
    uint64_t max_bucket_entries_;
    // Size of each entry
    uint16_t entry_size_;
    // Bucket determined by the first "log_num_buckets" bits starting at "begin_bits"
    uint32_t begin_bits_;
    // Log of the number of buckets; num bits to use to determine bucket
    uint32_t log_num_buckets_;

    std::vector<bucket_t> buckets_;

    uint64_t prev_bucket_buf_size = 0;
    uint64_t prev_bucket_index_ = 0;
    uint64_t prev_bucket_position_start = 0;
    uint64_t prev_bucket_position_offset = 0;

    bool done = false;

    uint64_t final_position_start = 0;
    uint64_t final_position_end = 0;
    uint64_t next_bucket_to_sort = 0;
    std::unique_ptr<uint8_t[]> entry_buf_;
    strategy_t strategy_;

    void SortBucket()
    {
        if (!sorting_memory_) {
            // preallocation only needed for
            // in FreeMemory() or the destructor
            uint64_t const max_entries = memory_per_bucket_ / entry_size_;
            max_sorting_memory_ = Util::RoundSize(max_entries) * entry_size_;
            std::cout << "!!! GIVE ME MEMORY !!!" << max_sorting_memory_ / 1024 / 1024 / 1024 << std::endl;
            sorting_memory_.reset(new uint8_t[max_sorting_memory_]);
        }
        this->done = true;
        if (next_bucket_to_sort >= buckets_.size()) {
            throw InvalidValueException("Trying to sort bucket which does not exist.");
        }
        uint64_t const bucket_i = this->next_bucket_to_sort;
        bucket_t& b = buckets_[bucket_i];

        uint64_t const bucket_entries = b.write_pointer / entry_size_;
        assert(bucket_entries == b.num_entries);

        if (b.num_entries * entry_size_ > memory_per_bucket_) {
            std::cout << "Not enough memory in this bucket!" << std::endl;
            std::cout << "Num entries: " << b.num_entries << std::endl;
            std::cout << "Req memory MB(per bucket): " << (b.num_entries * entry_size_ / 1024 / 1024) << std::endl;
            std::cout << "Req memory MB(total): " << (log_num_buckets_ * log_num_buckets_ * b.num_entries * entry_size_ / 1024 / 1024) << std::endl;
            throw InvalidStateException("SortBucket: Per bucket out of memory bounds write attempt");
        }

        uint64_t const bucket_offset = bucket_i * memory_per_bucket_;
        uint64_t const entries_fit_in_memory = memory_per_bucket_ / entry_size_;

        double const have_ram = entry_size_ * entries_fit_in_memory / (1024.0 * 1024.0 * 1024.0);
        double const qs_ram = entry_size_ * bucket_entries / (1024.0 * 1024.0 * 1024.0);
        double const u_ram =
            Util::RoundSize(bucket_entries) * entry_size_ / (1024.0 * 1024.0 * 1024.0);

        if (bucket_entries > entries_fit_in_memory) {
            throw InsufficientMemoryException(
                "Not enough memory for sort in memory. Need to sort " +
                std::to_string(b.write_pointer / (1024.0 * 1024.0 * 1024.0)) +
                "GiB");
        }
        bool const last_bucket = (bucket_i == buckets_.size() - 1)
            || buckets_[bucket_i + 1].write_pointer == 0;

        bool const force_quicksort = (strategy_ == strategy_t::quicksort)
            || (strategy_ == strategy_t::quicksort_last && last_bucket);

        // Do SortInMemory algorithm if it fits in the memory
        // (number of entries required * entry_size_) <= total memory available
        if (!force_quicksort &&
            Util::RoundSize(bucket_entries) * entry_size_ <= max_sorting_memory_) {
            std::cout << "\tBucket " << bucket_i << " uniform sort. Items: " << bucket_entries << " Ram: " << std::fixed
                      << std::setprecision(3) << have_ram << "GiB, u_sort min: " << u_ram
                      << "GiB, qs min: " << qs_ram << "GiB." << std::endl;
            UniformSort::SortMemoryToMemory(
                memory_start_.get() + bucket_offset,
                sorting_memory_.get(),
                entry_size_,
                bucket_entries,
                used_bitset,
                begin_bits_ + log_num_buckets_);
        } else {
            // Are we in Compress phrase 1 (quicksort=1) or is it the last bucket (quicksort=2)?
            // Perform quicksort if so (SortInMemory algorithm won't always perform well), or if we
            // don't have enough memory for uniform sort
            std::cout << "\tBucket " << bucket_i << " QS. Items: " << bucket_entries << " Ram: " << std::fixed
                      << std::setprecision(3) << have_ram << "GiB, u_sort min: " << u_ram
                      << "GiB, qs min: " << qs_ram << "GiB. force_qs: " << force_quicksort
                      << std::endl;
            QuickSort::Sort(memory_start_.get() + bucket_offset, entry_size_, bucket_entries, begin_bits_ + log_num_buckets_);
        }

        this->final_position_start = this->final_position_end;
        this->final_position_end += b.write_pointer;
        this->next_bucket_to_sort += 1;
    }
};

#endif  // SRC_CPP_FAST_SORT_ON_DISK_HPP_
