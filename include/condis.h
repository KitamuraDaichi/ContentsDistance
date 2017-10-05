#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <graph_manager.h>
#include <iostream>
#include <sstream>
#include <mysql/mysql.h>
#include <vector>
#include <TcpServer.h>

#define UPDATE_DISTANCE_FROM_CS 16
#define UPDATE_DISTANCE_FROM_GM 17

void *cd_thread(void *arg);

class I_DbAccess {

public:
	I_DbAccess() {}
	~I_DbAccess() {}
};
class MysqlAccess : public I_DbAccess {
	MYSQL	*conn;
	MYSQL	mysql;
	MYSQL_RES	*result;
	MYSQL_ROW	record;
	my_bool	reconnect;
public:
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
    int server_port;
  
  public:
    ~UpdateDistance();
    UpdateDistance();
    Tcp_Server *ts;
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

struct message_from_cs {
  struct node_id dest_id;
  int hop;
  double cfec_part_value;
}; 

