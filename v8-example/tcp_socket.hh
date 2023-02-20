#pragma once

#include <atomic>
#include <memory>
#include <optional>
#include <string>

extern "C" {
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
}

class TCPSocket {
public:
    TCPSocket(const TCPSocket &socket);

    TCPSocket(TCPSocket &&socket);

    ~TCPSocket();

    TCPSocket& operator=(const TCPSocket &other);

    static std::optional<TCPSocket> open(const std::string &address, short port);

    std::optional<TCPSocket> accept() const;

    operator int() const { return fd; }

    void write(const std::string &msg) const;

private:
    TCPSocket(int fd)
        : fd(fd),
          shared_count(std::make_shared<std::atomic<int>>(1)) {}

    int fd;
    std::shared_ptr<std::atomic<int>> shared_count;
};