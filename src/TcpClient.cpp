#include <TcpClient.h>

TcpClient::TcpClient()
{
  sock_flag = 0;
}

TcpClient::~TcpClient()
{
  if (sock_flag == 1) {
    (void) close(soc);
  }
}

  int
TcpClient::InitClientSocket(const char *hostnm, const char *portnm)
{
  char nbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
  struct addrinfo hints, *res0;
  int errcode;

  (void) memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;


  if ((errcode = getaddrinfo(hostnm, portnm, &hints, &res0)) != 0) {
    std::cerr << "getaddrinfo():" << gai_strerror(errcode) << std::endl;
    return -1;
  }

  if ((errcode = getnameinfo(res0->ai_addr, res0->ai_addrlen, nbuf, sizeof(nbuf), sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV)) != 0) {
    std::cerr << "getnameinfo():" << gai_strerror(errcode) << std::endl;
    return -1;
  }
//#ifdef _DEBUG
  std::cout << "addr=" << nbuf << std::endl;
  std::cout << "port=" << sbuf << std::endl;
//#endif

  if ((soc = socket(res0->ai_family, res0->ai_socktype, res0->ai_protocol)) == -1) {
    perror("socket");
    freeaddrinfo(res0);
    return -1;
  }

  sock_flag = 1;

  if (connect(soc, res0->ai_addr, res0->ai_addrlen) == -1) {
    perror("connect");
    (void) close(soc);
    freeaddrinfo(res0);
    return -1;
  }
  freeaddrinfo(res0);
  return soc;
}

  int
TcpClient::SendMsg(char *buf, size_t buf_size)
{
  int len;

  if ((len = send(soc, buf, buf_size, 0)) == -1) {
    perror("send");
    return -1;
  }

  return len;
}

  int
TcpClient::RecvMsg(char *buf, size_t buf_size)
{
  int len;

  if ((len = recv(soc, buf, buf_size, 0)) == -1) {
    perror("recv");
    return -1;
  }

  return len;
}

  int
TcpClient::RecvMsgAll(char *buf, size_t buf_size)
{
  int len = 0;

  while (len < (int)buf_size) {
    len += recv(soc, buf + len, buf_size - len, 0);
    if (len == 0) {
#ifdef _DEBUG
      std::cerr << "Error: Connection disconnected" << std::endl;
#endif
      return len;
    }
  }
  return len;
}
