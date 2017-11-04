#ifndef INCLUDE_TCP_CLIENT_H
#define INCLUDE_TCP_CLIENT_H

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sysexits.h>
#include <unistd.h>

class TcpClient {
  private:
    int soc;
    int sock_flag;
  public:
    TcpClient();
    ~TcpClient();
    int InitClientSocket(const char *hostnm, const char *portnm);
    int SendMsg(char *buf, size_t buf_size);
    int RecvMsg(char *buf, size_t buf_size);
    int RecvMsgAll(char *buf, size_t buf_size);
};

#endif
