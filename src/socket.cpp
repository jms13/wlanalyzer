/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Samsung Electronics
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <fcntl.h>
#include "socket.h"
#include "common.h"

UnixLocalSocket::UnixLocalSocket() : _fd(-1), _connected(false)
{
}

UnixLocalSocket::~UnixLocalSocket()
{
    if (isConnected())
    {
        disconnectFromServer();
    }
}

UnixLocalSocketError UnixLocalSocket::connectToServer(const char *path)
{
    std::string strPath(path);
    return connectToServer(strPath);
}

UnixLocalSocketError UnixLocalSocket::connectToServer(const std::string &path)
{
    int err;

    if (path.empty())
        return ServerNotFoundError;

    if (isConnected())
        disconnectFromServer();

    _fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (check_error(_fd))
        return SocketResourceError;

    sockaddr_un address;
    memset(&address, 0, sizeof(sockaddr_un));
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, path.c_str(), path.size());
    err = connect(_fd, (sockaddr *)&address, sizeof(sockaddr_un));
    if (err == -1)
    {
        disconnectFromServer();

        switch (errno)
        {
        case EINVAL:
        case ECONNREFUSED:
            return ConnectionRefusedError;
        case ENOENT:
            return ServerNotFoundError;
        case ETIMEDOUT:
            return SocketTimeoutError;

        default:
            return UnknownSocketError;
        }
    }

    Logger::getInstance()->log("Connected to %s\n", path.c_str());
    _connected = true;

    return NoError;
}

UnixLocalSocketError UnixLocalSocket::disconnectFromServer()
{
    if (_connected)
    {
        close(_fd);
        _fd = -1;

        _connected = false;
    }
}

void UnixLocalSocket::setSocketDescriptor(int fd, int flags)
{
    if (isConnected())
        disconnectFromServer();

    if (flags)
    {
        fcntl(fd, F_SETFL, flags);
    }

    _fd = fd;
    _connected = true;
}

bool operator==(int fd, const UnixLocalSocket &sock)
{
    return fd == sock.getSocketDescriptor();
}

long UnixLocalSocket::read(char *data, long max_size) const
{
    if (!isConnected())
        return -1;

    int err = recv(_fd, data, max_size, MSG_DONTWAIT);
    if (err == -1 && (errno == EWOULDBLOCK || errno == EAGAIN))
        return 0;

    return err;
}

bool UnixLocalSocket::write(const char *data, long c) const
{
    while (c > 0)
    {
        int wrote = send(_fd, data, c, 0);
        if (wrote == -1)
            return false;

        c -= wrote;
    }

    return true;
}
