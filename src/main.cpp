#include <condis.h>
//#include <TcpServer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <OnMemoryDatabase.h>
//#include <graph_manager.h>
//#include <iostream>
//#include <sstream>
//#include <mysql/mysql.h>
//#include <vector>
//#include <memory>
//#include <mysql_driver.h>
//#include <mysql_connection.h>
//#include <mysql_error.h>
//#include <cppconn/Statement.h>
//#include <cppconn/ResultSet.h>
OnMemoryDatabase *omd;
MysqlAccess global_db;
void *send_thread(void *arg) {
  UpdateDistance *ud;
  ud = new UpdateDistance();
  ud->setupUpdate3();
}

void *start_point_start(void *arg) {
  int init_sleep_time = INIT_SLEEP_TIME;
  int sleep_time = SLEEP_TIME;
  sleep(init_sleep_time);
  while(1) {
    pthread_t send_th;
    pthread_create(&send_th, NULL, send_thread, NULL);
    sleep(sleep_time);
    pthread_join(send_th, NULL);
  }
}


int main() {
  std::cout << "debug 3" << std::endl;
  if (global_db.connectDb("localhost", "root", "", "cfec_database2") < 0) {
    fprintf(stderr, "Databaseにconnectできませんでした。\n");
  }
  std::cout << "debug 1" << std::endl;
  omd = new OnMemoryDatabase();
  std::cout << "debug 2" << std::endl;
  //omd->testLoadIp();
  //return 0;

  pthread_t send_thread;
  pthread_create(&send_thread, NULL, start_point_start, NULL);

  //ここから前のやつ
  UpdateDistance *ud;
  int server_port = 5566;

  ud = new UpdateDistance();
  ud->start(server_port);

  pthread_join(send_thread, NULL);

  /*
  std::string own_id = "aaaa";
  std::string other_id = "bbbb";
  if (ud->distance(own_id, other_id) < 0) {
    std::cerr << "このコンテンツはこのサーバの管理下ではありません。" << std::endl;
  }
  */
  //ここまで前のやつ
  /*
  ts = new Tcp_Server();
  ts->init(server_port);
  ts->startTcpServer();

  ts->acceptLoop(cs_thread, &db, sizeof(MysqlAccess *));

  char query[256] = "show tables";

  if (db.connectDb("localhost", "root", "hige@mos", "contents_distance") < 0) {
    fprintf(stderr, "Databaseにconnectできませんでした。\n");
  }
  if (db.sendQuery(query) < 0) {
    fprintf(stderr, "Databaseをreadできませんでした。\n");
  }
  char **result = db.getResult();

  std::cout << result[0] << std::endl;

	printf("OK bokujou\n");
  */

	return 0;
}
