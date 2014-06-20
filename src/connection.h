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

#ifndef CONNECTION_H
#define CONNECTION_H

#include <pthread.h>
#include <stack>
#include <ev++.h>
#include "socket.h"
#include "common.h"
#include "message.h"

class WlaIODumper;
class WlaProxyServer;

class WlaConnection
{
public:
    WlaConnection(WlaProxyServer *parent, WlaIODumper *writer);
    ~WlaConnection();

    // TODO: handle connection close
    void createConnection(UnixLocalSocket client, UnixLocalSocket server);

private:
    void handleConnection(ev::io &watcher, int revents);
    WlaMessageBuffer *handleRead(UnixLocalSocket &src, UnixLocalSocket &dst);
    void handleWrite(UnixLocalSocket &dst, std::stack<WlaMessageBuffer *> &msgStack);

    void closeConnection();

private:
    UnixLocalSocket client;
    UnixLocalSocket wayland;

    WlaProxyServer *parent;
    WlaIODumper *writer;

    std::stack<WlaMessageBuffer *> events;
    std::stack<WlaMessageBuffer *> requests;
};

#endif // CONNECTION_H
