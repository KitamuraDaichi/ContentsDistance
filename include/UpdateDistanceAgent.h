#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <graph_manager.h>
#include <iostream>
#include <sstream>
#include <mysql/mysql.h>
#include <vector>
#include <TcpServer.h>

class UpdateDistanceAgent {
  private: 
    MysqlAccess db;
    int server_port;

  public:
    ~UpdateDistanceAgent();
    UpdateDistanceAgent(struct client_data cdata);
    Tcp_Server *ts;
    void updateDistanceFromCs();
    void updateDistanceFromGm();
    int distance(std::string own_id, std::string other_id);
};
