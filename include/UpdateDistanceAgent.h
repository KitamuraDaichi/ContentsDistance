#ifndef UPDATEDISTANCEAGENT
#define UPDATEDISTANCEAGENT
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

#define MAXHOP 3

class UpdateDistanceAgent {
  private: 
    MysqlAccess db;
    int server_port;
    ReadConfig *rc;

  public:
    ~UpdateDistanceAgent();
    UpdateDistanceAgent(struct client_data cdata);
    Tcp_Server *ts;
    std::map<std::string, std::vector<struct message_and_next_content_id> > dest_compless_map;
    int updateCvalueNeighbor();
    int updateCvalueNeighborOnMemory();
    int updateMysqlFromMemory(struct c_values recv_column, std::string recv_time_stamp);
    int addNeighborNodeFromMemory(struct c_values new_column);
    int addCValueFromMemory(struct c_values new_column, std::string recv_time_stamp, int same_flag);
    int existSameRouteColumnOnMemory(std::string ownci, std::string othci, std::string pathc);
    int deleteColumnCValueOnMemory(std::string ownci, std::string othci, std::string pathc);
    int addNeighborNodes(char *own_content_id, char *other_content_id, char *version_id);
    int addCValue(char *own_content_id, char *other_content_id, char *version_id, int hop, double next_value, char *value_chain, char *node_chain, std::string recv_time_stamp);
    int existSameNeighborNode(char *own_content_id, char *other_content_id);
    int existSameRouteColumn(char *own_content_id, char *other_content_id, char *path_chain);
    int deleteColumnNeighborNode(char *own_content_id, char *other_content_id);
    int deleteColumnCValue(char *own_content_id, char *other_content_id, char *path_chain);
    int nodeDegree(char *content_id);
    void outputTime(int start_end);
    int updateDistanceFromCs();
    void updateDistanceFromGm();
    int distance(std::string own_id, std::string other_id);
    int addDb(char *own_content_id, char *other_content_id, char *version_id, double *value_chain, int hop, char *path_chain, std::string recv_time_stamp);
    int existColumn(char *content_id);
    int deleteColumn(char *own_content_id, char *other_content_id, char *path_chain);
    int propagateUpdate();
    int calculateDistances(struct node_id other_content_id);
};

std::string id_to_string(struct node_id id);
std::string int_to_string(int num);
std::string double_to_string(double num);
void *propagate_thread(void *arg);
struct propagate_thread_arg {
  struct message_second ms;
  int agent_num;
  int thread_num;
};

struct message_and_next_content_id {
  char *n_n_c_p;
  char next_content_id[34];
  int degree;
};
#endif
