#include <condis.h>
#include <UpdateDistanceAgent.h>
//#include <TcpServer.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <graph_manager.h>
//#include <iostream>
//#include <sstream>
//#include <mysql/mysql.h>
//#include <vector>

MysqlAccess::MysqlAccess() {
  this->initBuf();
}

MysqlAccess::~MysqlAccess() {
  mysql_close(&mysql);
  if (result != NULL) {
    mysql_free_result(result);
  }
}

int MysqlAccess::initBuf() {
  reconnect = 1;

  mysql_init(&mysql);
  mysql_options(&mysql, MYSQL_SET_CHARSET_NAME, "utf8");
  mysql_options(&mysql, MYSQL_OPT_RECONNECT, &reconnect);
  result = NULL;

  return 0;
}

int MysqlAccess::connectDb(const char* serverAddr, const char* userName, const char* passwd, const char* dbName) {
  //this->conn = mysql_real_connect(&mysql, serverAddr, userName, passwd, dbName, MYSQL_PORT, MYSQL_SOCK, 0);
  this->conn = mysql_real_connect(&mysql, serverAddr, userName, passwd, dbName, MYSQL_PORT, NULL, 0);
  if (this->conn == NULL) {
    return -1;
  }
  return 0;
}

int MysqlAccess::sendQuery(char *sql) {
  int ret;
  if (result != NULL) {
    mysql_free_result(result);
    result = NULL;
  }
  if ((ret = mysql_query(&mysql, sql)) != 0) {
    return -1;
  }
  result = mysql_store_result(&mysql);
  if (result == NULL) {
    return 0;
  }
  return mysql_num_rows(result);
}

void MysqlAccess::freeBuf() {
  mysql_free_result(result);
}

int MysqlAccess::getRowNum() {
  return mysql_num_rows(this->result);
}
char** MysqlAccess::getResult() {
  if (this->result != NULL) {
    return mysql_fetch_row(this->result);
  } else {
    return NULL;
  }
}
const char* MysqlAccess::getErrow() {
  return mysql_error(this->conn);
}
/*
   std::string getString() {
   resp = mysql_use_result(&(this->mysql));

   }
   */
/*
   int readDb() {
   char sql_str[255];
   snprintf(&sql_str[0], sizeof(sql_str) - 1, "select * from tb_test");
   if(mysql_query(&mysql, &sql_str[0])) {
   mysql_close(conn);
   exit(-1);
   }
   */


UpdateDistance::~UpdateDistance() {
  delete &db;
  //delete ts;
}
UpdateDistance::UpdateDistance() {
  if (db.connectDb("localhost", "root", "hige@mos", "contents_distance") < 0) {
    fprintf(stderr, "Databaseにconnectできませんでした。\n");
  }
}
int UpdateDistance::start(int port_num) {
  this->server_port = port_num;

  ts = new Tcp_Server();
  ts->init(this->server_port);
  ts->startTcpServer();
  ts->acceptLoop(cd_thread, this, sizeof(UpdateDistance *));

  return 0;
}
void UpdateDistance::updateDistanceFromCs() {
  std::cout << "in updateDistanceFromCs" << std::endl;
}
void UpdateDistance::updateDistanceFromGm() {
  std::cout << "in updateDistanceFromGm" << std::endl;
}


int UpdateDistance::distance(std::string own_id, std::string other_id) {
  std::string query;
  query = "select * from own_contents where own_content_id = \"" + own_id + "\" ;";
  //query = "show tables;";

  if (this->db.sendQuery((char *)query.c_str()) < 0) {
    fprintf(stderr, "Databaseをreadできませんでした。\n");
  }
  char **result = this->db.getResult();
  if (result == NULL) {
    return -1;
  }

  std::cout << result[0] << std::endl;

  return 0;
}
void *cd_thread(void *arg) {
  std::cout << "in cd_thread" << std::endl;
  UpdateDistance *ud;
  struct client_data cdata;
  ud = ((struct cd_thread_arg *)arg)->ud;

  std::cout << "in cd_thread" << std::endl;
  struct message_header rmsg_h;

  pthread_mutex_t *accept_sock_mutex = ((struct thread_arg *)arg)->amp;
  pthread_cond_t *p_signal = ((struct thread_arg *)arg)->p_signal;
  cdata = ((struct thread_arg *)arg)->cdata;

	pthread_mutex_lock(accept_sock_mutex); // [******** lock ********]
	pthread_cond_signal(p_signal); // [******* cond signal *******]
	pthread_mutex_unlock(accept_sock_mutex); // [******** unlock ********]

  UpdateDistanceAgent *uda = new UpdateDistanceAgent(cdata);

  for (;;) {
    if (uda->ts->recvMsgAll((char *)&rmsg_h, sizeof(struct message_header)) == 0) {
      break;
    } else {
      rmsg_h.convert_ntoh();
    }

    if (rmsg_h.code == UPDATE_DISTANCE_FROM_CS) {
      std::cout << "rmsg_h.code: " << (int)rmsg_h.code << endl;
      uda->updateDistanceFromCs();
    } else if (rmsg_h.code == UPDATE_DISTANCE_FROM_GM) {
      std::cout << "rmsg_h.code: " << (int)rmsg_h.code << endl;
      uda->updateDistanceFromGm();
    } else {
      std::cout << "=> Code not found: " << rmsg_h.code << std::endl;
    }
  }

  return NULL;
}

void *cs_thread(void *arg)
{
  std::cout << "in cs_thread" << std::endl;
  return NULL;
}

