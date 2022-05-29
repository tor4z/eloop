#include <cerrno>
#include <sys/uio.h>
#include "eloop/buffer.hpp"


namespace eloop
{

const char Buffer::kCRLF[] = "\r\n";


ssize_t Buffer::readFD(int fd, int *savedErrno)
{
    char extra_buffer[65536];
    struct iovec vec[2];
    const size_t writable = writableBytes();

    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;
    vec[1].iov_base = extra_buffer;
    vec[1].iov_len = sizeof(extra_buffer);

    const int iovcnt = (writable < sizeof(extra_buffer)) ? 2: 1;
    const ssize_t n = readv(fd, vec, iovcnt);

    if(n < 0)
    {
        *savedErrno  = errno;
    }
    else if (static_cast<size_t>(n) <= writable)
    {
        // only iov[0]
        writerIndex_ += n;
    }
    else
    {
        // only iov[0] and iov[1]
        writerIndex_ = buffer_.size();
        append(extra_buffer, n - writable);
    }

    return n;
}

}
