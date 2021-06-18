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

#ifndef SRC_CPP_UNIFORMSORT_HPP_
#define SRC_CPP_UNIFORMSORT_HPP_

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "./disk.hpp"
#include "./util.hpp"

namespace UniformSort {

    inline int64_t const BUF_SIZE = 262144;

    inline static bool IsPositionEmpty(const uint8_t *memory, uint32_t const entry_len)
    {
        for (uint32_t i = 0; i < entry_len; i++)
            if (memory[i] != 0)
                return false;
        return true;
    }

    inline void SortToMemory(
        FileDisk &input_disk,
        uint64_t const input_disk_begin,
        uint8_t *const memory,
        uint32_t const entry_len,
        uint64_t const num_entries,
        uint32_t const bits_begin)
    {
        uint64_t const memory_len = Util::RoundSize(num_entries) * entry_len;
        auto const swap_space = std::make_unique<uint8_t[]>(entry_len);
        auto const buffer = std::make_unique<uint8_t[]>(BUF_SIZE);
        uint64_t bucket_length = 0;
        // The number of buckets needed (the smallest power of 2 greater than 2 * num_entries).
        while ((1ULL << bucket_length) < 2 * num_entries) bucket_length++;
        memset(memory, 0, memory_len);

        uint64_t read_pos = input_disk_begin;
        uint64_t buf_size = 0;
        uint64_t buf_ptr = 0;
        uint64_t swaps = 0;
        for (uint64_t i = 0; i < num_entries; i++) {
            if (buf_size == 0) {
                // If read buffer is empty, read from disk and refill it.
                buf_size = std::min((uint64_t)BUF_SIZE / entry_len, num_entries - i);
                buf_ptr = 0;
                input_disk.Read(read_pos, buffer.get(), buf_size * entry_len);
                read_pos += buf_size * entry_len;
            }
            buf_size--;
            // First unique bits in the entry give the expected position of it in the sorted array.
            // We take 'bucket_length' bits starting with the first unique one.
            uint64_t pos =
                Util::ExtractNum(buffer.get() + buf_ptr, entry_len, bits_begin, bucket_length) *
                entry_len;
            // As long as position is occupied by a previous entry...
            while (!IsPositionEmpty(memory + pos, entry_len) && pos < memory_len) {
                // ...store there the minimum between the two and continue to push the higher one.
                if (Util::MemCmpBits(
                        memory + pos, buffer.get() + buf_ptr, entry_len, bits_begin) > 0) {
                    memcpy(swap_space.get(), memory + pos, entry_len);
                    memcpy(memory + pos, buffer.get() + buf_ptr, entry_len);
                    memcpy(buffer.get() + buf_ptr, swap_space.get(), entry_len);
                    swaps++;
                }
                pos += entry_len;
            }
            // Push the entry in the first free spot.
            memcpy(memory + pos, buffer.get() + buf_ptr, entry_len);
            buf_ptr += entry_len;
        }
        uint64_t entries_written = 0;
        // Search the memory buffer for occupied entries.
        for (uint64_t pos = 0; entries_written < num_entries && pos < memory_len;
             pos += entry_len) {
            if (!IsPositionEmpty(memory + pos, entry_len)) {
                // We've found an entry.
                // write the stored entry itself.
                memcpy(
                    memory + entries_written * entry_len,
                    memory + pos,
                    entry_len);
                entries_written++;
            }
        }

        assert(entries_written == num_entries);
    }

    inline void SortMemoryToMemory(
        uint8_t *const src,
        uint8_t *const buf_,
        uint32_t const entry_len,
        uint64_t const num_entries,
        auto used_bitset,
        uint32_t const bits_begin)
    {
        uint64_t const rounded_entries = Util::RoundSize(num_entries);
        if (rounded_entries > used_bitset.size()) {
            std::cout << "Error: Uniform sort: Bitset size not big enough, minimum: " << rounded_entries << std::endl;
            exit(1);
        }
        // assert(num_entries < bitset_size);
        // if (num_entries > bitset_size) {
        //     std::cout << "Too many entires per bucket. Increase the number of buckets!";
        //     throw InvalidValueException("Too many entires per bucket. Increase the number of buckets!");
        // }
        
        // uint64_t const memory_len = Util::RoundSize(num_entries) * entry_len;

        auto const swap_space = std::make_unique<uint8_t[]>(entry_len);
        uint8_t *const swap_space_start = swap_space.get();
        uint64_t bucket_length = 0;
        // The number of buckets needed (the smallest power of 2 greater than 2 * num_entries).
        while ((1ULL << bucket_length) < 2 * num_entries) bucket_length++;
        // std::chrono::steady_clock::time_point begin_new = std::chrono::steady_clock::now();
        // is reset needed?
        // memset(buf_, 0, memory_len);
        
        // std::chrono::steady_clock::time_point end_new = std::chrono::steady_clock::now();
        // std::cout << "        alloc time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end_new - begin_new).count() << "[ms]" << std::endl;

        // uint64_t buf_ptr = 0;
        // uint64_t swaps = 0;
        uint64_t buf_ofs = 0;

        for (uint64_t i = 0; i < num_entries; i++) {
            // First unique bits in the entry give the expected position of it in the sorted array.
            // We take 'bucket_length' bits starting with the first unique one.

            // uint64_t pos =
            //     Util::ExtractNum(src + buf_ptr, entry_len, bits_begin, bucket_length) *
            //     entry_len;
            uint64_t idx =
                Util::ExtractNum(src + buf_ofs, entry_len, bits_begin, bucket_length);

            uint64_t mem_ofs = idx * entry_len;
            // As long as position is occupied by a previous entry...
            // while (!IsPositionEmpty(buf_ + pos, entry_len) && pos < memory_len) {
            //     // ...store there the minimum between the two and continue to push the higher one.
            //     if (Util::MemCmpBits(
            //             buf_ + pos, src + buf_ptr, entry_len, bits_begin) > 0) {
            //         memcpy(swap_space_start, buf_ + pos, entry_len);
            //         memcpy(buf_ + pos, src + buf_ptr, entry_len);
            //         memcpy(src + buf_ptr, swap_space_start, entry_len);
            //         swaps++;
            //     }
            //     pos += entry_len;
            // }
            if (bits_begin == 0) {
                // std::cout << idx << ".";
                while ((used_bitset[idx] == 1) && idx < rounded_entries) {
                    // ...store there the minimum between the two and continue to push the higher one.
                    if (memcmp(buf_ + mem_ofs, src + buf_ofs, entry_len) > 0) {
                        memcpy(swap_space_start, buf_ + mem_ofs, entry_len);
                        memcpy(buf_ + mem_ofs, src + buf_ofs, entry_len);
                        memcpy(src + buf_ofs, swap_space_start, entry_len);
                    }
                    idx++;
                    mem_ofs += entry_len;
                }
            } else {
                // std::cout << idx << "."; 
                while ((used_bitset[idx] == 1) && idx < rounded_entries) {
                    // ...store there the minimum between the two and continue to push the higher one.
                    if (Util::MemCmpBits(
                            buf_ + mem_ofs, src + buf_ofs, entry_len, bits_begin) > 0) {
                        memcpy(swap_space_start, buf_ + mem_ofs, entry_len);
                        memcpy(buf_ + mem_ofs, src + buf_ofs, entry_len);
                        memcpy(src + buf_ofs, swap_space_start, entry_len);
                    }
                    idx++;
                    mem_ofs += entry_len;
                }
            }
            // Push the entry in the first free spot.
            // memcpy(buf_ + pos, src + buf_ptr, entry_len);
            // buf_ptr += entry_len;

            memcpy(buf_ + mem_ofs, src + buf_ofs, entry_len);
            used_bitset[idx] = 1;
            buf_ofs += entry_len;
        }

        // 
        uint64_t entries_written = 0;
        uint64_t entries_ofs = 0;
        // Search the memory buffer for occupied entries.
        // for (uint64_t pos = 0; entries_written < num_entries && pos < memory_len;
        //      pos += entry_len) {
        //     if (!IsPositionEmpty(buf_ + pos, entry_len)) {
        //         // We've found an entry.
        //         // write the stored entry itself.
        //         memcpy(
        //             src + entries_written * entry_len,
        //             buf_ + pos,
        //             entry_len);
        //         entries_written++;
        //     }
        // }
        for (uint64_t idx = 0, mem_ofs = 0; entries_written < num_entries && idx < rounded_entries;
             idx++, mem_ofs += entry_len) {
            if (used_bitset[idx] == 1) {
                used_bitset[idx] = 0;
                // We've found an entry.
                // write the stored entry itself.
                memcpy(
                    src + entries_ofs,
                    buf_ + mem_ofs,
                    entry_len);
                entries_written++;
                entries_ofs += entry_len;
            }
        }

        // memcpy(src, buf_, memory_len);

        assert(entries_written == num_entries);
    }

}

#endif  // SRC_CPP_UNIFORMSORT_HPP_
