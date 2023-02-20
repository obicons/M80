#include "tcp_socket.hh"

std::optional<TCPSocket> TCPSocket::open(const std::string &address, short port) {
    int socket_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        return {};
    }

    struct sockaddr_in addr = {
        .sin_family = PF_INET,
        .sin_port = static_cast<in_port_t>(htons(port)),
    };

    if (!inet_aton(address.c_str(), &addr.sin_addr)) {
        close(socket_fd);
        return {};
    }

    if (bind(socket_fd, (struct sockaddr *) &addr, sizeof(addr)) != 0) {
        close(socket_fd);
        return {};
    }

    if (listen(socket_fd, 1024) == -1) {
        close(socket_fd);
        return {};
    }

    return TCPSocket(socket_fd);
}

TCPSocket::~TCPSocket() {
    if (this->shared_count != nullptr &&
        this->shared_count.get()->fetch_sub(1) == 1) {
        close(this->fd);
    }
}

TCPSocket::TCPSocket(const TCPSocket &other) {
    other.shared_count.get()->fetch_add(1);
    this->shared_count = other.shared_count;
    this->fd = other.fd;
}

TCPSocket::TCPSocket(TCPSocket &&other) {
    this->shared_count = other.shared_count;
    this->fd = other.fd;
    other.shared_count = nullptr;
}

TCPSocket& TCPSocket::operator=(const TCPSocket &other) {
    other.shared_count.get()->fetch_add(1);
    this->shared_count = other.shared_count;
    this->fd = other.fd;
    return *this;
}

std::optional<TCPSocket> TCPSocket::accept() const {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    int fd = ::accept(this->fd, (struct sockaddr *) &addr, &len);
    if (fd == -1) {
        return {};
    }
    return TCPSocket(fd);
}

void TCPSocket::write(const std::string &msg) const {
    ::write(this->fd, msg.c_str(), msg.length());
}
