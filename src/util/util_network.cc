#include <util_linux.hh>

#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#include <cstring>
#include <iostream>

namespace eve
{
static struct addrinfo *__getaddrinfo(const std::string &address, unsigned short port);

int set_fd_nonblock(int fd)
{
    if (fd < 0)
    {
        LOG_ERROR << " : error fd < 0";
        return -1;
    }

    if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
    {
        LOG_ERROR << ": fcntl set nonblock err fd = " << fd;
        return -1;
    }

    if (fcntl(fd, F_SETFD, 1) == -1)
    {
        LOG_ERROR << ": fcntl setfd err fd = " << fd;
        return -1;
    }

    return 0;
}

int get_nonblock_socket()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        LOG_ERROR << " socket err\n";
        return -1;
    }
    if (set_fd_nonblock(fd) == -1)
        return -1;
    return fd;
}

static struct addrinfo *__getaddrinfo(const std::string &address, unsigned short port)
{
    std::string strport = std::to_string(port);

    struct addrinfo *aitop, hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; /* turn nullptr host name into INADDR_ANY */

    int ai_result = getaddrinfo(address.c_str(), strport.c_str(), &hints, &aitop);

    if (ai_result != 0)
    {
        if (ai_result == EAI_SYSTEM)
            LOG_ERROR << " getaddrinfo err\n";
        else
            LOG_ERROR << " getaddrinfo err with " << gai_strerror(ai_result);
        return nullptr;
    }
    return aitop;
}

int bind_socket(const std::string &address, unsigned short port, int reuse)
{
    struct addrinfo *aitop = nullptr;
    int on;

    if (address.empty() && port == 0)
        aitop = nullptr, reuse = 0; /* just create an unbound socket */
    else
    {
        aitop = __getaddrinfo(address, port);
        if (!aitop)
            return -1;
    }

    int fd = get_nonblock_socket();
    if (fd == -1)
        goto out;

    on = 1;
    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&on, sizeof(on));
    if (reuse)
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *)&on, sizeof(on));

    if (aitop)
    {
        if (bind(fd, aitop->ai_addr, aitop->ai_addrlen) == -1)
        {
            LOG_ERROR << "bind socket error";
            goto out;
        }
    }

    freeaddrinfo(aitop);
    return fd;

out:
    freeaddrinfo(aitop);
    close(fd);
    return -1;
}

/** @accept
 *  return host and port and nfd
 */
int accept_socket(int fd, std::string &host, int &port)
{
    struct sockaddr_storage ss_client;
    struct sockaddr *sa = (struct sockaddr *)&ss_client;
    socklen_t client_addrlen = sizeof(ss_client);

    int sockfd = accept(fd, (struct sockaddr *)&ss_client, &client_addrlen);

    if (sockfd == -1)
    {
        if (errno != EAGAIN && errno != EINTR)
            LOG_ERROR << ": accept err errno=" << errno;
        return -1;
    }
    if (set_fd_nonblock(sockfd) == -1)
        return -1;

    char ntop[NI_MAXHOST];
    char strport[NI_MAXSERV];

    int ni_result = getnameinfo(sa, client_addrlen,
                                ntop, sizeof(ntop), strport, sizeof(strport),
                                NI_NUMERICHOST | NI_NUMERICSERV);

    if (ni_result != 0)
    {
        if (ni_result == EAI_SYSTEM)
            LOG_ERROR << " getnameinfo err";
        else
            LOG_ERROR << " getnameinfo err with " << gai_strerror(ni_result);
        return -1;
    }

    host = std::string(ntop);
    port = std::stoi(strport);
    return sockfd;
}

int socket_connect(int fd, const std::string &address, unsigned short port)
{
    struct addrinfo *ai = __getaddrinfo(address, port);

    if (!ai)
    {
        LOG_ERROR << " error get addrinfo: " << address << ":" << port;
        return -1;
    }

    if (connect(fd, ai->ai_addr, ai->ai_addrlen) == -1)
    {
        if (errno != EINPROGRESS)
        {
            LOG_ERROR << " err with errno=" << errno;
            freeaddrinfo(ai);
            return -1;
        }
    }
    LOG << " succeed connect to " << address << ":" << port;

    freeaddrinfo(ai);
    return 0;
}

std::pair<int, int> get_fdpair()
{
    int pairfd[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, pairfd) == -1)
        LOG_ERROR << " socketpair error\n";

    // std::cout << "[FD] " << __func__ << " " << pairfd[0] << "," << pairfd[1] << std::endl;
    return std::make_pair(pairfd[0], pairfd[1]);
}

int listenfd(int fd)
{
    if (listen(fd, 128) == -1)
    {
        LOG_ERROR << "listen error";
        close(fd);
        return -1;
    }
    return 0;
}

int check_socket(int socket)
{
    int error;
    if (getsockopt(socket, SOL_SOCKET, SO_ERROR, (void *)&error, (socklen_t *)sizeof(error)) == -1)
    {
        LOG_ERROR << ": getsockopt err with errno=" << errno;
        return -1;
    }
    if (error)
    {
        LOG_ERROR << ": err with " << std::strerror(error);
        return -1;
    }
    return 0;
}

int http_connect(const std::string &address, unsigned short port)
{
    struct addrinfo *aitop = __getaddrinfo(address, port);
    if (!aitop)
        return -1;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
        LOG_ERROR << "socket failed";

    // std::cout << "[FD] " << __func__ << " fd=" << sockfd << std::endl;
    if (connect(sockfd, aitop->ai_addr, aitop->ai_addrlen) == -1)
        LOG_ERROR << "connec failed";
    freeaddrinfo(aitop);
    return sockfd;
}

} // namespace eve
