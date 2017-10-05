#include <condis.h>
#include <TcpServer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <graph_manager.h>
#include <iostream>
#include <sstream>
#include <mysql/mysql.h>
#include <vector>
//#include <memory>
//#include <mysql_driver.h>
//#include <mysql_connection.h>
//#include <mysql_error.h>
//#include <cppconn/Statement.h>
//#include <cppconn/ResultSet.h>





int main() {
  UpdateDistance *ud;
  MysqlAccess db;
  Tcp_Server *ts_main;
  ts_main = new Tcp_Server();
  int server_port = 5566;
  ts_main->init(server_port);

  ud = new UpdateDistance();
  ud->start(server_port);

  std::string own_id = "aaaa";
  std::string other_id = "bbbb";
  if (ud->distance(own_id, other_id) < 0) {
    std::cerr << "このコンテンツはこのサーバの管理下ではありません。" << std::endl;
  }
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
