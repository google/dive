/*
 Copyright 2018 Google Inc.
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 http://www.apache.org/licenses/LICENSE-2.0
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
*/

#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <memory>
#include <vector>

namespace Dive
{
// LinearAllocator allocates memory on a single block. Thus the address returned are
// with in One block and is continuous. The memory block and its size is provided when initiate the
// allocator. This allocator is mainly used in dive capture layer to serialize the parameters into
// one block so that they can be inserted into the dive capture file directly or indirectly via NOP
// packets. It can also be used in the decoder to print out the parameters, or store them in memory
// (need to make sure the provide block not be re-allocated, otherwise the pointer deserialized will
// be invalid).
template<size_t alignment = 8> class LinearAllocator
{
public:
    static constexpr size_t kAlignment = alignment;

    LinearAllocator(size_t mem_size) :
        owned_mem_(std::make_unique<char[]>(mem_size)),
        mem_(owned_mem_.get()),
        total_size_(mem_size),
        head_(mem_)
    {}

    LinearAllocator(char* mem, size_t mem_size) : mem_(mem), total_size_(mem_size), head_(mem_) {}

    void* Alloc(const size_t size)
    {
        if (size == 0)
        {
            return nullptr;
        }

        uintptr_t h = (uintptr_t)head_;
        char*     alloc = (char*)((h + kAlignment - 1) & ~(kAlignment - 1));
        if (alloc + size > mem_ + total_size_)
        {
            return nullptr;
        }

        head_ = alloc + size;
        allocated_size_ += static_cast<size_t>((uintptr_t)head_ - h);
        return alloc;
    }

    size_t AllocatedSize() { return allocated_size_; }

    void Reset()
    {
        head_ = mem_;
        allocated_size_ = 0;
    }

private:
    std::unique_ptr<char[]> owned_mem_;
    char*                   mem_ = nullptr;
    const size_t            total_size_ = 0;
    size_t                  allocated_size_ = 0;
    char*                   head_ = nullptr;
};

template<typename T> constexpr bool IsPowerOf2(T x)
{
    return ((x > 0) && (0 == (x & (x - 1))));
}

template<size_t alignment, typename T> std::enable_if_t<std::is_integral_v<T>, T> AlignUp(T x)
{
    static_assert(IsPowerOf2(alignment), "Power of 2 required");
    return (x + alignment - 1) & ~(alignment - 1);
}

template<size_t alignment, typename T> std::enable_if_t<std::is_pointer_v<T>, T> AlignUp(T p)
{
    static_assert(IsPowerOf2(alignment), "Power of 2 required");
    uintptr_t x = reinterpret_cast<uintptr_t>(p);
    uintptr_t aligned = (x + alignment - 1) & ~(alignment - 1);
    return reinterpret_cast<T>(aligned);
}

template<size_t alignment, typename T> std::enable_if_t<std::is_pointer_v<T>, bool> IsAligned(T p)
{
    static_assert(IsPowerOf2(alignment), "Power of 2 required");
    return ((reinterpret_cast<uintptr_t>(p) & (alignment - 1)) == 0);
}

//
// DynamicLinearAllocator is a block based linear allocator for fast allocation of
// simple data structures.
//
// Unlike using a std::vector as a linear allocator the block based approach
// means that re-allocation will not occur when resizing the vector.  This is
// needed so that pointers may be serialized internally from this allocator.
//
// Memory is managed in fixed sized blocks, when a block is full a new block
// will be created. The block will be the size of the larger of the requested
// allocation (+ alignment padding) and the default block size.
//
// Destructors will not be called when Reset.
//
template<size_t default_block_size = 1024 * 32, size_t alignment = 8> class DynamicLinearAllocator
{
    static_assert(IsPowerOf2(alignment), "Power of 2 required");

public:
    static constexpr size_t kDefaultBlockSize = default_block_size;
    static constexpr size_t kAlignment = alignment;

    DynamicLinearAllocator() { blocks_.emplace_back(kDefaultBlockSize); }

    DynamicLinearAllocator(size_t initial_capacity) { blocks_.emplace_back(initial_capacity); }

    void* Alloc(const size_t size)
    {
        allocated_size_ = AlignUp<kAlignment>(allocated_size_) + size;

        assert(blocks_.size() > 0);

        // Try and alloc from the active block.
        auto alloc = blocks_.back().Alloc(size);
        if (alloc)
        {
            return alloc;
        }

        // The current block is full, try another.

        if (blocks_.back().FreeSpace() > kDefaultBlockSize / 4 || size >= kDefaultBlockSize)
        {
            // There is still a lot of free space in the current block, or the requested allocation
            // is large enough to require a dedicated block. Put the requested allocation in a
            // dedicated block, and maintain the current active block.
            blocks_.emplace_back(size);
            alloc = blocks_.back().Alloc(size);
            std::swap(blocks_[blocks_.size() - 2], blocks_[blocks_.size() - 1]);
        }
        else
        {
            // Start a new current active block.
            blocks_.emplace_back(kDefaultBlockSize);
            alloc = blocks_.back().Alloc(size);
        }
        assert(alloc);
        return alloc;
    }

    size_t AllocatedSize() { return allocated_size_; }

    void Reset()
    {
        // free all but the first block
        blocks_.resize(1);

        // reset the first block
        blocks_.back().Reset();

        allocated_size_ = 0;
    }

    size_t NumBlocksAllocated() const { return blocks_.size(); }

    // This function is not actually used right now.
    // This is just a demonstration of how the allocated data could be copied/written as if it was
    // in one contiguous allocation, even though it is actually split between several blocks.
    void Copy(size_t offset, void* dst, size_t size)
    {
        auto it = blocks_.begin();
        for (; it != blocks_.end() && offset >= it->AllocatedSpace(); ++it)
            offset -= it->AllocatedSpace();
        for (; it != blocks_.end() && size >= it->AllocatedSpace() - offset; ++it)
        {
            memcpy(dst, it->Data() + offset, it->AllocatedSpace() - offset);
            size -= it->AllocatedSpace() - offset;
            offset = 0;
        }
        if (size > 0)
            memcpy(dst, it->Data() + offset, size);
    }

private:
    class Block
    {
    public:
        Block(size_t blocksize)
        {
            blocksize_ = blocksize;
            auto data = static_cast<uint8_t*>(
            ::operator new[](blocksize_, static_cast<std::align_val_t>(kAlignment)));
            data_.reset(data);
            Reset();
        }
        Block() : Block(kDefaultBlockSize) {}

        void* Alloc(const size_t size)
        {
            // head_ should always be aligned
            assert(IsAligned<kAlignment>(head_));

            uint8_t* alloc = head_;
            if (alloc + size > data_.get() + blocksize_)
            {
                return nullptr;
            }

            // advance head past the allocation, and align
            head_ = AlignUp<kAlignment>(alloc + size);

            return alloc;
        }

        void Reset() { head_ = data_.get(); }

        size_t AllocatedSpace() const
        {
            // head_ should always be aligned
            assert(IsAligned<kAlignment>(head_));
            assert(head_ >= data_.get());

            return static_cast<size_t>(head_ - data_.get());
        }

        size_t BlockSize() const { return blocksize_; }

        size_t FreeSpace() const
        {
            assert(AllocatedSpace() <= BlockSize());
            return BlockSize() - AllocatedSpace();
        }

        void* Data() const { return data_.get(); }

    private:
        struct Deleter
        {
            void operator()(uint8_t* p) const
            {
                if (p != nullptr)
                {
                    ::operator delete[](p, static_cast<std::align_val_t>(kAlignment));
                }
            }
        };
        size_t                            blocksize_;
        uint8_t*                          head_;
        std::unique_ptr<uint8_t, Deleter> data_;
    };

private:
    size_t             allocated_size_ = 0;
    std::vector<Block> blocks_;
};

}  // namespace Dive