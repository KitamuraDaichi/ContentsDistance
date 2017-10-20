#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <graph_manager.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <mysql/mysql.h>
#include <vector>
#include <TcpServer.h>
#include <functional>
#include <map>

class UpdateDistanceAgent {
  private: 
    MysqlAccess db;
    int server_port;
    ReadConfig *rc;

  public:
    ~UpdateDistanceAgent();
    UpdateDistanceAgent(struct client_data cdata);
    Tcp_Server *ts;
    int updateDistanceFromCs();
    void updateDistanceFromGm();
    int distance(std::string own_id, std::string other_id);
    int addDB(struct node_id own_content_id, struct node_id other_content_id, int hop, double value);
    int existColumn(struct node_id own_id);
    int propagateUpdate(struct node_id own_content_id);
    int calculateDistances(struct node_id other_content_id);
};

std::string id_to_string(struct node_id id);
std::string int_to_string(int num);
std::string double_to_string(double num);
