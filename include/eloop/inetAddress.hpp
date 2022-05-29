#pragma once

#include <string>
#include <netinet/in.h>

namespace eloop
{

class InetAddress
{
public:
    explicit InetAddress(uint16_t port = 0, bool loopback = false);
    InetAddress(const std::string &ip, uint16_t port);

    void setAddress(const struct sockaddr_in &addr)
    {
        addr_ = addr;
    }

    const struct sockaddr *getSockAddr() const
    {
        return reinterpret_cast<const struct sockaddr*>(&addr_);
    }

    socklen_t getSockLen() const
    {
        return sizeof(addr_);
    }

    std::string toIP() const;
    uint16_t toPort() const;
    std::string toIPPort() const;
private:
    struct sockaddr_in addr_;
};

}
