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

#define UPDATE_DISTANCE_FROM_CS 16
#define UPDATE_DISTANCE_SECOND 17
#define INITIAL_VALUE 1000.0

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
    Tcp_Server *ts;
    TcpClient *tc;
    ReadConfig *rc;
    //std::vector<struct neighbor_node_column> vec_neighbor_node_column;
    struct neighbor_node_column *nncp;
    int start(int port_num);
    void updateDistanceFromCs();
    void updateDistanceFromGm();
    int distance(std::string own_id, std::string other_id);
};
void *cd_thread(void *arg);
void *cs_thread(void *arg);

struct cd_thread_arg {
  UpdateDistance *ud;
};

struct neighbor_node_column {
  char own_content_id[33];
  char other_content_id[33];
  char version_id[25];
};
struct message_to_neighbor_nodes {
  std::string start_content_id;
  std::string version_id;
  int hop;
  std::string value_chain;
  std::string node_chain;
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
