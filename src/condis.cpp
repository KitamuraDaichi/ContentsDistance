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
  free(this->nncp);
  //delete ts;
}
UpdateDistance::UpdateDistance() {
  rc = new ReadConfig("catalog_distance.conf");
  if (db.connectDb("localhost", "root", "hige@mos", "cfec_database") < 0) {
    fprintf(stderr, "Databaseにconnectできませんでした。\n");
  }
}
int UpdateDistance::setupUpdate() {
  std::cout << "in setupUpdate" << std::endl;
  std::string query;
  query = "select * from neighbor_nodes";
  int tmp;
  if ((tmp = this->db.sendQuery((char *)query.c_str())) < 0) {
    std::cerr << "sendQuery返り値: " << tmp << std::endl;
    return -1;
  }

  // mysqlから隣接ノードテーブルを読み出し
  MYSQL_ROW row;
  int count = mysql_num_rows(this->db.result);
  if ((this->nncp = (struct neighbor_node_column *)malloc(count * sizeof(struct neighbor_node_column))) == NULL) {
    std::cout << "error" << std::endl;
    exit(1);
  }
  std::cout << "count: " << count << std::endl;
  std::cout << "std_size: " << (sizeof(this->nncp) / sizeof(nncp[0])) << std::endl;
  std::cout << "struct neighbor_node size: " << sizeof(struct neighbor_node_column) << std::endl;
  
  int i = 0;
  while ((row = mysql_fetch_row(this->db.result))) {
    struct neighbor_node_column nnc;
    strcpy(nnc.own_content_id, row[0]);
    strcpy(nnc.other_content_id, row[1]);
    strcpy(nnc.version_id, row[2]);
    memcpy(&(this->nncp[i]), &nnc, sizeof(neighbor_node_column));
    i++;
  }
  int j;
  for (j = 0; j < count; j++) {
    std::cout << "own_id: " << this->nncp[j].own_content_id << std::endl;
    std::cout << "other_id: " << this->nncp[j].other_content_id << std::endl;
    std::cout << "version_id: " << this->nncp[j].version_id << std::endl;

 /*
  *   * data format and where pointers are pointing
  *   *
  *   * s/r_buf
  *   * ------------------------------------------------------------------------------
  *   * | mes type | mes header | neighbor_node_column | hop | value_chain | node_chain|
  *   * -------------------------------------------------------------------------------
  *   * A          A            A                      A     A             A___ s_n_c
  *   * |          |            |                      |     \___ s/r_v_c
  *   * |          |            |                      \___ s/r_hop
  *   * |          |            \___ s_n_column
  *   * |          \___ s_n_header
  *   * |
  *   * \___ s/r_header
  *   */

    int hop = 1;
    ostringstream os;
    os << INITIAL_VALUE;
    double c_value = INITIAL_VALUE;

    char s_buf[sizeof(message_header) 
      + sizeof(neighbor_node_column) + sizeof(int) + (sizeof(double) * hop)];

    struct message_header *s_header = (struct message_header *)s_buf;
    struct neighbor_node_column *s_n_column = (struct neighbor_node_column *)(s_header + sizeof(struct message_header));
    int *s_hop = (int *)(s_n_column + sizeof(struct neighbor_node_column));
    double *s_v_c = (double *)(s_hop + sizeof(int));
    s_v_c = (double *)malloc(sizeof(double) * hop);
    //char s_n_c[hop][33];
    //s_n_c = (char *)(s_v_c + sizeof(double) * hop)
    std::cout << "debug 1" << std::endl;

    setupMsgHeader(s_header, UPDATE_DISTANCE, 0, 0);
    memcpy(s_n_column, &(this->nncp[j]), sizeof(struct neighbor_node_column));
    *s_hop = hop;
    std::cout << "debug 2" << std::endl;
    for (int k = 0; k < hop; k++) {
      s_v_c[k] = INITIAL_VALUE;
    }
    std::cout << "debug 3" << std::endl;

    std::string id_first = nncp[j].other_content_id;
    id_first = id_first.substr(0, 8);
    std::cout << "debug 4: " << id_first << std::endl;

    this->tc = new TcpClient();

    in_port_t gm_port;
    std::string ip;
    os.str("");
    os.clear(stringstream::goodbit);
    rc->getParam("CONTENT_DISTANCE_PORT", &gm_port);
    os << gm_port;
    std::string str_gm_port = os.str();
    rc->getParam(id_first, &ip);
    std::cout << "ip: " << ip << std::endl;
    std::cout << "s_buf: " << s_buf << std::endl;
    this->tc->InitClientSocket(ip.c_str(), str_gm_port.c_str());
    this->tc->SendMsg((char *)s_buf, sizeof(s_buf));
    free(s_v_c);
    return 0;
    //this->tc->SendMsg((char *)str_buf.c_str(), str_buf.size());
  }

  // パケットの作成
  //struct message_to_neighbor_nodes mess; 
  //mess.start_node_id = row[0];
  //mess.version = 

  return 0;
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

    if (rmsg_h.code == UPDATE_DISTANCE) {
      std::cout << "rmsg_h.code: " << (int)rmsg_h.code << endl;
      uda->updateDistanceFromCs();
    } else if (rmsg_h.code == UPDATE_DISTANCE_SECOND) {
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

