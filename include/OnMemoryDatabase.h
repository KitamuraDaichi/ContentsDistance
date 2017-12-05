//#ifndef ON_MEMORY_DATABASE
//#define ON_MEMORY_DATABASE
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
#include <TcpClient.h>
#include <functional>
#include <map>
#include <time.h>
#include <condis.h>
#include <UpdateDistanceAgent.h>
#include <boost/thread.hpp>

class OnMemoryDatabase {
  private:
  public:
    MysqlAccess db;
    ~OnMemoryDatabase();
    OnMemoryDatabase();
    int loadMysql();
    int loadNeighborNodes();
    int loadIp();
    int loadCValues();
    int loadCValuesForSendByIp();
    int read();
    int write();
    int testLoadNeighborNodes();
    int testLoadIp();
    int testLoadCValues();
    int writeNeighborNodes(struct neighbor_nodes new_column);
    int writeIp(std::string new_ip);
    int writeCValues(struct c_values new_column);
    int copyBufferCValuesForSendByIp(char *s_buf, std::string ip, int start_num, int size);
    std::vector<std::string> next_ip_table;
    std::vector<struct neighbor_nodes> neighbor_nodes_table;
    std::vector<struct c_values> c_values_table;
    std::map<std::string, std::vector<struct neighbor_nodes> > neighbor_nodes_table_by_ip;
    std::map<std::string, std::vector<struct c_values> > c_values_by_own_content_id;
    std::map<std::string, struct c_values> c_values_by_start_path_end;
    std::map<std::string, std::vector<struct c_values> > c_values_for_send_by_ip;
};

struct neighbor_nodes {
  char own_content_id[CONTENT_ID_SIZE];
  char other_content_id[CONTENT_ID_SIZE];
  char version_id[VERSION_ID_SIZE];
  char next_server_ip[IP_SIZE];
};
struct c_values {
  char own_content_id[CONTENT_ID_SIZE];
  char other_content_id[CONTENT_ID_SIZE]; 
  char version_id[VERSION_ID_SIZE];
  int hop;
  double next_value;
  double value_chain[MAX_HOP];
  char node_chain[MAX_HOP - 1][CONTENT_ID_SIZE];
  char get_time[GET_TIME_SIZE];
};
//#endif
