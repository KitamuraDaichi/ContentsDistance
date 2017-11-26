#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <graph_manager.h>
#include <iostream>
#include <sstream>
#include <mysql/mysql.h>
#include <vector>
#include <TcpServer.h>
#include <TcpClient.h>
#include <pthread.h>
#include <unistd.h>
#include <map>

#define LOCAL_SERVER_IP "10.58.58.2"
#define INIT_SLEEP_TIME 3 
#define SLEEP_TIME      86400 
#define UPDATE_DISTANCE_FROM_CS 16
#define UPDATE_DISTANCE_SECOND 17
#define UPDATE_DISTANCE 18
#define INITIAL_VALUE 1000.0
#define CONTENT_ID_SIZE 34
#define VERSION_ID_SIZE 26
#define IP_SIZE 34
#define MAX_HOP 3
#define VALUE_SIZE 10
#define MAX_COLUMN_NUM 100
#define PROPAGATION_THREAD_NUM 4

void *cd_thread(void *arg);

class I_DbAccess {

public:
	I_DbAccess() {}
	~I_DbAccess() {}
};
class MysqlAccess : public I_DbAccess {
	MYSQL	*conn;
	MYSQL_ROW	record;
	my_bool	reconnect;
public:
	MYSQL	mysql;
	MYSQL_RES	*result;
	MysqlAccess();
	~MysqlAccess();

	int initBuf();
  int connectDb(const char* serverAddr, const char* userName, const char* passwd, const char* dbName);
	int sendQuery(char *sql);
  void freeBuf();
  int getRowNum();
  char** getResult();
  const char* getErrow();
};

class UpdateDistance {
  private: 
    MysqlAccess db;
    char *neighbor_node_list;
    int server_port;
  
  public:
    ~UpdateDistance();
    UpdateDistance();

    int setupUpdate();
    int setupUpdate2();
    Tcp_Server *ts;
    TcpClient *tc;
    ReadConfig *rc;
    //std::vector<struct neighbor_node_column> vec_neighbor_node_column;
    std::map<std::string, std::vector<struct neighbor_node_column> > dest_compless_map;
    //std::map<std::string, std::string>dest_compless_map;
    struct neighbor_node_column *nncp;
    int start(int port_num);
    void updateDistanceFromCs();
    void updateDistanceFromGm();
    int distance(std::string own_id, std::string other_id);
};
void *cd_thread(void *arg);
void *cs_thread(void *arg);
void *send_each_ip(void *arg);

struct cd_thread_arg {
  UpdateDistance *ud;
};

struct neighbor_node_column {
  char own_content_id[34];
  char other_content_id[34];
  char version_id[26];
};
struct message_to_neighbor_nodes_header {
  int value_chain_size;
  int node_chain_size;
};

struct message_to_neighbor_nodes {
  char start_content_id[33];
  char next_content_id[33];
  char version_id[25];
  int hop;
  char *value_chain;
  char *node_chain;
};

struct message_from_cs {
  struct node_id source_id;
  struct node_id dest_id;
  int hop;
  double cfec_part_value;
}; 

struct message_second {
  struct node_id source_id;
  struct node_id dest_id;
  int hop;
  double cfec_part_value;
}; 

struct send_each_ip_arg {
  char *ip;
  int max_ip_num;
  int *next_ip_num; 
};
