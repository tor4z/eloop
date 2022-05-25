#pragma once

#include <algorithm>
#include <vector>
#include <string>
#include <cassert>
#include <cstring>


namespace eloop
{

class Buffer
{
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;
    explicit Buffer(size_t initialSize)
        : buffer_(initialSize + kCheapPrepend),
          readerIndex_(kCheapPrepend),
          writerIndex_(kCheapPrepend)
    {

    }

    void swap(Buffer &target)
    {

    }

    size_t readableBytes() const
    {
        return writerIndex_ - readerIndex_;
    }

    size_t writebleBytes() const
    {
        return buffer_.size() - writerIndex_;
    }

    size_t prependableBytes() const
    {
        return readerIndex_;
    }

    const char *peek() const
    {
        return begin() + readerIndex_;
    }
private:
    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
    static const char kCRLF[];

    char *begin()
    {
        return &*buffer_.begin();
    }

    const char *begin() const
    {
        return &*buffer_.begin();
    }

    void makeSpace(size_t len)
    {

    }
};

}
