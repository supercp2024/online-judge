#pragma once
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <memory>

#include "my_utility.hpp"

namespace ns_socket
{
    inline sockaddr_in internetStructInit(sa_family_t sin_family, short port_h, const char* ip_h)
    {
        sockaddr_in internet_socket_addr;
        std::uninitialized_fill_n(reinterpret_cast<char*>(&internet_socket_addr), sizeof(sockaddr_in), '\0');
        internet_socket_addr.sin_family = sin_family;
        internet_socket_addr.sin_port = htons(port_h);
        in_addr ip_n;
        if (inet_aton(ip_h, &ip_n) == 0)
        {
            throw ns_interface::interface_exception{ "invalid ip string" };
        }
        internet_socket_addr.sin_addr = ip_n;
        return internet_socket_addr;
    }

    inline sockaddr_in internetStructInit(sa_family_t sin_family, short port_h, in_addr_t ip_n)
    {
        sockaddr_in internet_socket_addr;
        std::uninitialized_fill_n(reinterpret_cast<char*>(&internet_socket_addr), sizeof(sockaddr_in), '\0');
        internet_socket_addr.sin_family = sin_family;
        internet_socket_addr.sin_port = htons(port_h);
        internet_socket_addr.sin_addr.s_addr = ip_n;
        return internet_socket_addr;
    }
}