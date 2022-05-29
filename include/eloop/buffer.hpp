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
        assert(readableBytes() == 0);
        assert(writableBytes() == initialSize);
        assert(prependableBytes() == kCheapPrepend);
    }

    void swap(Buffer &target)
    {
        buffer_.swap(target.buffer_);
        std::swap(readerIndex_, target.readerIndex_);
        std::swap(writerIndex_, target.writerIndex_);
    }

    size_t readableBytes() const
    {
        return writerIndex_ - readerIndex_;
    }

    size_t writableBytes() const
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

    const char *findCRLF() const
    {
        const char *crlf = std::search(
            peek(), beginWrite(), kCRLF, kCRLF + 2
        );
        return crlf == beginWrite() ? nullptr: crlf;
    }

    const char *findCRLF(const char *start) const
    {
        assert(peek() <= start);
        assert(start <= beginWrite());
        const char *crlf = std::search(
            start, beginWrite(), kCRLF, kCRLF + 2
        );
        return crlf == beginWrite()? nullptr: crlf;
    }

    const char *findEOL() const
    {
        const void *eol = memchr(peek(), '\n', readableBytes());
        return static_cast<const char*>(eol);
    }

    const char *findEOL(const char *start) const
    {
        assert(start <= beginWrite());
        assert(start >= peek());
        const void *eol = memchr(start, '\n', beginWrite() - start);
        return static_cast<const char*>(eol);
    }

    void retrieveAll()
    {
        // abandon all buffer
        readerIndex_ = kCheapPrepend;
        writerIndex_ = kCheapPrepend;
    }

    void retrieve(size_t len)
    {
        assert(len <= readableBytes());
        if(len < readableBytes())
            readerIndex_ += len;
        else
            retrieveAll();
    }

    void retrieveUntil(const char *end)
    {
        assert(peek() <= end);
        assert(end <= beginWrite());
        retrieve(end - peek());
    }

    void retrieveInt64()
    {
        retrieve(sizeof(int64_t));
    }

    void retrieveInt32()
    {
        retrieve(sizeof(int32_t));
    }

    void retrieveInt16()
    {
        retrieve(sizeof(int16_t));
    }

    void retrieveInt8()
    {
        retrieve(sizeof(int8_t));
    }

    std::string retrieveAsString(size_t len)
    {
        assert(len < readableBytes());
        std::string result(peek(), len);
        retrieve(len);
        return result;
    }

    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());
    }

    void ensureWritableBytes(size_t len)
    {
        if (writableBytes() < len)
        {
            makeSpace(len);
        }
        assert(writableBytes() >= len);
    }

    void hasWritten(size_t len)
    {
        assert(len < writableBytes());
        writerIndex_ += len;
    }

    void append(const char *data, size_t len)
    {
        ensureWritableBytes(len);
        std::copy(
            data,
            data + len,
            beginWrite()
        );
        hasWritten(len);
    }

    void append(const void *data, size_t len)
    {
        append(static_cast<const char*>(data), len);
    }

    void append(std::string_view data)
    {
        append(data.begin(), data.size());
    }

    void append(const std::string &data)
    {
        append(data.c_str(), data.size());
    }

    void appendInt64(int64_t x)
    {
        int64_t be64 = htobe64(x);
        append(&be64, sizeof(be64));
    }

    void appendInt32(int32_t x)
    {
        int32_t be32 = htobe32(x);
        append(&be32, sizeof(be32));
    }

    void appendInt16(int16_t x)
    {
        int16_t be16 = htobe16(x);
        append(&be16, sizeof(be16));
    }

    void appendInt8(int8_t x)
    {
        append(&x, sizeof(x));
    }

    int64_t peekInt64()
    {
        assert(readableBytes() >= sizeof(int64_t));
        int64_t be64 = 0;
        std::memcpy(&be64, peek(), sizeof(be64));
        return be64toh(be64);
    }

    int32_t peekInt32()
    {
        assert(readableBytes() >= sizeof(int32_t));
        int32_t be32 = 0;
        std::memcpy(&be32, peek(), sizeof(be32));
        return be32toh(be32);
    }

    int16_t peekInt16()
    {
        assert(readableBytes() >= sizeof(int16_t));
        int16_t be16 = 0;
        std::memcpy(&be16, peek(), sizeof(be16));
        return be16toh(be16);
    }

    int8_t peekInt8()
    {
        assert(readableBytes() >= sizeof(int8_t));
        int8_t x = 0;
        std::memcpy(&x, peek(), sizeof(x));
        return x;
    }

    int64_t readInt64()
    {
        int64_t x = peekInt64();
        retrieveInt64();
        return x;
    }

    int32_t readInt32()
    {
        int32_t x = peekInt32();
        retrieveInt32();
        return x;
    }

    int16_t readInt16()
    {
        int16_t x = peekInt16();
        retrieveInt16();
        return x;
    }

    int8_t readInt8()
    {
        int8_t x = peekInt8();
        retrieveInt8();
        return x;
    }

    char *beginWrite()
    {
        return begin() + writerIndex_;
    }

    const char *beginWrite() const
    {
        return begin() + writerIndex_;
    }

    ssize_t readFD(int fd, int *savedErrno);

    void prepend(const void *data, size_t len)
    {
        assert(len <= prependableBytes());
        readerIndex_ -= len;
        auto d = static_cast<const char*>(data);
        std::copy(
            d,
            d + len,
            begin() + readerIndex_
        );
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
        if (writableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            buffer_.resize(writerIndex_ + len);
        }
        else
        {
            // reuse space
            assert(kCheapPrepend < readerIndex_);
            size_t readable = readableBytes();
            std::copy(
                begin() + readerIndex_,
                begin() + writerIndex_,
                begin() + kCheapPrepend
            );
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
            assert(readable == readableBytes());
        }
    }
};

}
