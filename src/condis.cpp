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


UpdateDistance::~UpdateDistance() {
  delete &db;
  free(this->nncp);
  //delete ts;
}
UpdateDistance::UpdateDistance() {
  rc = new ReadConfig("catalog_distance.conf");
  if (db.connectDb("localhost", "root", "", "cfec_database2") < 0) {
    fprintf(stderr, "Databaseにconnectできませんでした。\n");
  }
}
int UpdateDistance::setupUpdate2() {
  std::cout << "in setupUpdate2" << std::endl;
  std::string query;
  // mysqlから隣接ノードテーブルを読み出し
  query = "select distinct next_server_ip from neighbor_nodes";
  int tmp;
  if ((tmp = this->db.sendQuery((char *)query.c_str())) < 0) {
    std::cerr << "sendQuery返り値: " << tmp << std::endl;
    return -1;
  }
  int ip_num = mysql_num_rows(this->db.result);
  char ip[ip_num][IP_SIZE];
  int i = 0;
  MYSQL_ROW row;
  while ((row = mysql_fetch_row(this->db.result))) {
    strcpy(ip[i], row[0]);
    i++;
  }
	pthread_t thread_id[ip_num];
  char thread_arg[ip_num][IP_SIZE];
  for (i = 0; i < ip_num; i++) {
    memcpy(thread_arg[i], (char *)ip[i], sizeof(char) * IP_SIZE);
    pthread_create(&thread_id[i], NULL, send_each_ip, (void *)thread_arg[i]); 
    pthread_join(thread_id[i], NULL);
  }
  for (i = 0; i < ip_num; i++) {
    //pthread_join(thread_id[i], NULL);
  }
  return 0;
}
int UpdateDistance::setupUpdate() {
  std::cout << "in setupUpdate" << std::endl;
  std::string query;
  // mysqlから隣接ノードテーブルを読み出し
  query = "select * from neighbor_nodes";
  int tmp;
  if ((tmp = this->db.sendQuery((char *)query.c_str())) < 0) {
    std::cerr << "sendQuery返り値: " << tmp << std::endl;
    return -1;
  }

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

  // サーバ圧縮
  int j;
  for (j = 0; j < count; j++) {
    std::string ip;
    std::string id_first = nncp[j].other_content_id;
    id_first = id_first.substr(0, 8);
    rc->getParam(id_first, &ip);

    if (this->dest_compless_map.count(ip) == 0) {
    }
    this->dest_compless_map[ip].push_back(nncp[j]);
  }
  // end サーバ圧縮
 /*
  *   * data format and where pointers are pointing
  *   *
  *   * s/r_buf
  *   * ------------------------------------------------------------------------------
  *   * |header|column num|hop|neighbor_node_column|value_chain|node_chain|
  *   * ------------------------------------------------------------------------------
  *   * A      A          A   A                    A           A
  *   * |      |          |   |                    |           |___ s_n_c
  *   * |      |          |   |                    |___ s_v_c
  *   * |      |          |   \___ s/r_n_n_c
  *   * |      |          \___ s/r_hop
  *   * |      \___ column_num_p
  *   * |          
  *   * |
  *   * \___ s/r_header
  *   */
  in_port_t gm_port;
  rc->getParam("CONTENT_DISTANCE_PORT", &gm_port);
  ostringstream os;
  os << gm_port;
  std::string str_gm_port = os.str();
  int hop = 1;
  // 圧縮されたテーブルを送信する
  std::map<std::string, vector<struct neighbor_node_column> >::iterator it;
  std::map<std::string, vector<struct neighbor_node_column> >::iterator itEnd 
    = dest_compless_map.end();
  for (it = dest_compless_map.begin(); it != itEnd; ++it) {
    std::cout << "ip: " << it->first << std::endl;
    std::vector<struct neighbor_node_column>::iterator v_itr;
    std::vector<struct neighbor_node_column>::iterator v_itrEnd = (it->second).end();

    int column_num = (it->second).size();
    char s_buf[sizeof(struct message_header) + sizeof(int) + sizeof(int)
      + (sizeof(neighbor_node_column) + (sizeof(double) * hop))
      * column_num];
    struct message_header *s_header = (struct message_header *)s_buf;
    setupMsgHeader(s_header, UPDATE_DISTANCE, 0, 0);
    int *column_num_p = (int *)((char *)s_buf + sizeof(struct message_header));
    *column_num_p = column_num;
    int *hop_p = (int *)((char *)column_num_p + sizeof(int));
    *hop_p = hop;
    char *column_start_p = (char *)((char *)hop_p + sizeof(int));

    for (v_itr = (it->second).begin(); v_itr != v_itrEnd; ++v_itr) {
      std::cout << "other_id: " << (*v_itr).other_content_id << std::endl;
      memcpy((struct neighbor_node_column *)column_start_p, &(*v_itr), sizeof(struct neighbor_node_column));
      double *s_v_c = (double *)((char *)column_start_p + sizeof(struct neighbor_node_column));
      for (int k = 0; k < hop; k++) {
        s_v_c[k] = INITIAL_VALUE;
      }
      column_start_p = (char *)((char *)s_v_c + sizeof(double) * hop);
    }
    std::cout << "debug 1" << std::endl;
    std::string ip = it->first;
    std::cout << "debug 2" << std::endl;

    if (ip != "10.58.58.2") {
      std::cout << "debug 4" << std::endl;
      TcpClient *tc2;
      tc2 = new TcpClient();
      if (tc2->InitClientSocket(ip.c_str(), str_gm_port.c_str()) == -1) {
        std::cout << "send error" << std::endl;
      }
      std::cout << "debug 3" << std::endl;
      tc2->SendMsg((char *)s_buf, sizeof(s_buf));
      std::cout << "debug 5" << std::endl;
      return 0;
    }
  }
  // end 圧縮されたテーブルを送信する

  // 行単位で送信する例
  for (j = 0; j < count; j++) {
    std::cout << "own_id: " << this->nncp[j].own_content_id << std::endl;
    std::cout << "other_id: " << this->nncp[j].other_content_id << std::endl;
    std::cout << "version_id: " << this->nncp[j].version_id << std::endl;

 /*
  *   * data format and where pointers are pointing
  *   *
  *   * s/r_buf
  *   * ------------------------------------------------------------------------------
  *   * | mes header | neighbor_node_column | hop | value_chain | node_chain|
  *   * ------------------------------------------------------------------------------
  *   * A            A                      A     A             A___ s_n_c
  *   * |            |                      |     \___ s/r_v_c
  *   * |            |                      \___ s/r_hop
  *   * |            \___ s_n_column
  *   * |          
  *   * |
  *   * \___ s/r_header
  *   */

    double c_value = INITIAL_VALUE;

    char s_buf[sizeof(int) 
      + sizeof(neighbor_node_column) + sizeof(int) + (sizeof(double) * hop)];

    struct message_header *s_header = (struct message_header *)s_buf;
    struct neighbor_node_column *s_n_column = (struct neighbor_node_column *)((char *)s_header + sizeof(int));
    int *s_hop = (int *)((char *)s_n_column + sizeof(struct neighbor_node_column));
    double *s_v_c = (double *)((char *)s_hop + sizeof(int));
    std::cout << "debug 1" << std::endl;

    setupMsgHeader(s_header, UPDATE_DISTANCE, 0, 0);
	
    memcpy(s_n_column, &(this->nncp[j]), sizeof(struct neighbor_node_column));

    std::cout << "neighbor size: " << sizeof(struct neighbor_node_column) << std::endl;

    *s_hop = hop;
    std::cout << "debug 2" << std::endl;
    for (int k = 0; k < hop; k++) {
      s_v_c[k] = INITIAL_VALUE;
    }
    std::cout << "debug 3" << std::endl;

    std::string id_first = nncp[j].other_content_id;
    id_first = id_first.substr(0, 8);

    this->tc = new TcpClient();

    in_port_t gm_port;
    std::string ip;
    rc->getParam("CONTENT_DISTANCE_PORT", &gm_port);
    std::cout << "debug 4: " << gm_port << std::endl;
    ostringstream os;
    os << gm_port;
    std::string str_gm_port = os.str();
    rc->getParam(id_first, &ip);
    std::cout << "ip: " << ip << std::endl;
    std::cout << "s_buf: " << s_buf << std::endl;

    std::cout << "s_n_column.own: " << s_n_column->own_content_id << std::endl;
    std::cout << "s_n_column.oth: " << s_n_column->other_content_id << std::endl;
    std::cout << "s_n_column.ver: " << s_n_column->version_id << std::endl;
	
    if (this->tc->InitClientSocket(ip.c_str(), str_gm_port.c_str()) == -1) {
      std::cout << "send error" << std::endl;
    }
    this->tc->SendMsg((char *)s_buf, sizeof(s_buf));
    return 0;
  }
  // end 行単位で送信する例

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
void *send_each_ip(void *arg) {
  char ip[IP_SIZE];
  memcpy(ip, (char *)arg, sizeof(char) * IP_SIZE);
  std::cout << "ip: " << ip << std::endl;
  std::string ip_s = ip;
  fflush(stdout);

  // 送信するデータの必要な量を知る
  MysqlAccess thread_db[MAX_HOP]; 
  int column_num[MAX_HOP];
  int i = 0;
  int send_size = 0;
    // headerとそれぞれのホップ数のカラム数
  send_size += sizeof(struct message_header) + sizeof(int) * MAX_HOP;
  for (i = 0; i < MAX_HOP; i++) {
    ostringstream os;
    os << i;
    std::string i_str = os.str();
    if (thread_db[i].connectDb("localhost", "root", "", "cfec_database2") < 0) {
      fprintf(stderr, "Databaseにconnectできませんでした。\n");
    }
    std::string query = "select * from c_values join neighbor_nodes on c_values.own_content_id=neighbor_nodes.own_content_id where next_server_ip=\"" + ip_s + "\" and c_values.hop=" + i_str + ";";
    int tmp;
    if ((tmp = thread_db[i].sendQuery((char *)query.c_str())) < 0) {
      std::cerr << "sendQuery返り値: " << tmp << std::endl;
      return NULL;
    }
    column_num[i] = mysql_num_rows(thread_db[i].result);
    /*
    send_size += (sizeof(struct neighbor_node_column) 
        + sizeof(double) + sizeof(char) * VALUE_SIZE * (i + 1) 
        + (sizeof(char) * (CONTENT_ID_SIZE + 1)) * (i + 1)) 
      * column_num[i];
      */
  }
  // end 送信するデータの必要な量を知る
  // headerだけ先に送る
  TcpClient *tc;
  tc = new TcpClient();
  if (tc->InitClientSocket(ip, "5566") == -1) {
    std::cout << "send error" << std::endl;
  }
  char s_head_buf[sizeof(struct message_header) + sizeof(int) * MAX_HOP];
  struct message_header *s_header = (struct message_header *)s_head_buf;
  setupMsgHeader(s_header, UPDATE_DISTANCE, 0, 0);
  int *column_num_start = (int *)((char *)s_header + sizeof(struct message_header));
  int *c_p = column_num_start;
  for (i = 0; i < MAX_HOP; i++) {
    *c_p = column_num[i];
    c_p = (int *)((char *)c_p + sizeof(int));
  }
  tc->SendMsg((char *)s_head_buf, sizeof(struct message_header) + sizeof(int) * MAX_HOP);
  // end headerだけ先に送る
  
  MYSQL_ROW row;
  for (i = 0; i < MAX_HOP; i++) {
    for (int total_col_num = 0; total_col_num < column_num[i]; total_col_num+=MAX_COLUMN_NUM) {
      int cn;
      if (total_col_num + MAX_COLUMN_NUM < column_num[i]) {
        cn = MAX_COLUMN_NUM;
      } else {
        cn = column_num[i] - total_col_num;
      }
      int buf_size = (sizeof(struct neighbor_node_column) 
        + sizeof(double) + sizeof(char) * VALUE_SIZE * (i + 1) 
        + (sizeof(char) * (CONTENT_ID_SIZE + 1)) * (i + 1)) 
      * cn;

      char s_buf[buf_size];
      struct neighbor_node_column *n_n_c_p = (struct neighbor_node_column *)s_buf;
      int j = 0;
      //while ((row = mysql_fetch_row(thread_db[i].result))) {
      for (int c = 0; c < cn; c++) {
        row = mysql_fetch_row(thread_db[i].result);
        strcpy(n_n_c_p->own_content_id, row[1]);// start_node
        strcpy(n_n_c_p->other_content_id, row[9]);// next_node
        strcpy(n_n_c_p->version_id, row[2]);
        std::string n_val_str = row[4];
        double *s_n_v = (double *)((char *)n_n_c_p + sizeof(struct neighbor_node_column));
        *s_n_v = atof(row[4]);//std::stod(n_val_str);
        char *s_v_c = (char *)((char *)s_n_v + sizeof(double));
        memcpy(s_v_c, (char *)row[5], sizeof(char) * VALUE_SIZE * (i + 1));
        char *s_n_c = (char *)((char *)s_v_c + sizeof(char) * VALUE_SIZE * (i + 1));
        std::string ownci_s = row[0];
        std::string node_chain_s = row[6];
        if (node_chain_s == "NULL") {
          memcpy(s_n_c, (char *)(ownci_s.c_str()), sizeof(char) * CONTENT_ID_SIZE);
          n_n_c_p = (struct neighbor_node_column *)((char *)s_n_c + sizeof(char) * CONTENT_ID_SIZE);
        } else {
          std::string new_node_chain_s = node_chain_s + "," + ownci_s;
          memcpy(s_n_c, (char *)(new_node_chain_s.c_str()), sizeof(char) * (CONTENT_ID_SIZE * (i + 1) + i));
          n_n_c_p = (struct neighbor_node_column *)((char *)s_n_c + sizeof(char) * (CONTENT_ID_SIZE * (i + 1) + i));
        }
      }
      char *debug_p = (char *)s_buf;
      while ((char *)debug_p < (char *)n_n_c_p) {
        struct neighbor_node_column *d_n_n_c_p = (struct neighbor_node_column *)debug_p;
        std::cout << "startid: " << d_n_n_c_p->own_content_id << std::endl;
        std::cout << "next_id: " << d_n_n_c_p->other_content_id << std::endl;
        std::cout << "versiid: " << d_n_n_c_p->version_id << std::endl;
        double *d_s_n_v = (double *)((char *)d_n_n_c_p + sizeof(struct neighbor_node_column));
        std::cout << "next_va: " << (*d_s_n_v) << std::endl;
        char *d_s_v_c = (char *)((char *)d_s_n_v + sizeof(double));
        std::cout << "val_cha: " << d_s_v_c << std::endl;
        char *d_s_n_c = (char *)((char *)d_s_v_c + sizeof(char) * VALUE_SIZE * (i + 1));
        std::cout << "nod_cha: " << d_s_n_c << std::endl;
        debug_p = (char *)((char *)d_s_n_c + sizeof(char) * (CONTENT_ID_SIZE));
      }
    }
  }
  std::cout << "end: " << ip << std::endl;
  return NULL;

  /*
    char own_id[own_num][CONTENT_ID_SIZE];
  MYSQL_ROW row;
  while ((row = mysql_fetch_row(thread_db.result))) {
    strcpy(own_id[i], row[0]);
    i++;
  }
  for (i = 0; i < own_num; i++) {
    std::cout << own_id[i] << std::endl;
    std::string own_id_s = own_id[i];
    query = "select * from c_values where own_content_id=\"" + own_id[i] + "\";";
    if ((tmp = thread_db.sendQuery((char *)query.c_str())) < 0) {
      std::cerr << "sendQuery返り値: " << tmp << std::endl;
      return NULL;
    }
    int colum_num = mysql_num_rows(thread_db.result);
  }
  // end送信するデータの必要な量を知る

  query = "select own_content_id,other_content_id from neighbor_nodes where next_server_ip=\"" + ip_s + "\";";
  if ((tmp = thread_db.sendQuery((char *)query.c_str())) < 0) {
    std::cerr << "sendQuery返り値: " << tmp << std::endl;
    return NULL;
  }
  int n_n_c_num = mysql_num_rows(thread_db.result);
  struct neighbor_node_column n_n_c[n_n_c_num];
  while ((row = mysql_fetch_row(thread_db.result))) {
    strcpy(n_n_c[i].own_content_id, row[0]);
    strcpy(n_n_c[i].other_content_id, row[1]);
    strcpy(n_n_c[i].version_id, row[2]);
    i++;
  }
  for (i = 0; i < n_n_c_num; i++) {
    std::cout << n_n_c[i].other_content_id << std::endl;
    std::string oth_id = n_n_c[i].other_content_id;
  }
  return NULL;
  */
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
      uda->updateCvalueNeighbor();
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

