#include <condis.h>
#include <UpdateDistanceAgent.h>
#include <OnMemoryDatabase.h>
//#include <TcpServer.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <graph_manager.h>
//#include <iostream>
//#include <sstream>
//#include <mysql/mysql.h>
//#include <vector>
using namespace boost;
extern OnMemoryDatabase *omd;
extern shared_mutex c_values_mutex;
extern shared_mutex vectors_mutex;

pthread_mutex_t propagation_thread_mutex = PTHREAD_MUTEX_INITIALIZER;
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
int UpdateDistance::setupUpdate3() {
  std::cout << "in setupUpdate3" << std::endl;
  // 送信開始の時間を出力
  char time_buff[] = "";
  time_t now = time(NULL);
  struct tm *pnow = localtime(&now);
  sprintf(time_buff, "%04d%02d%02d%02d%02d%02d", pnow->tm_year + 1900, pnow->tm_mon + 1, pnow->tm_mday, pnow->tm_hour, pnow->tm_min, pnow->tm_sec);
  std::string send_time = time_buff;
  ofstream fout("./send_start_time.csv", ios::app);
  fout << send_time << std::endl;
  fout.close();
  // 送信先のipのvectorでループを回す
  shared_lock <shared_mutex> read_lock(vectors_mutex);

  int max_ip_num = omd->next_ip_table.size();
  int start_thread_num = PROPAGATION_THREAD_NUM;
  int thread_num = 0;
  if (max_ip_num < PROPAGATION_THREAD_NUM) {
    thread_num = max_ip_num;
  } else {
    thread_num = PROPAGATION_THREAD_NUM;
  }
  std::string arr_ip[thread_num];
  pthread_t thread_id[thread_num];
  struct send_each_ip_on_memory_arg thread_arg[thread_num];
 
  std::map<std::string, vector<struct c_values> >::iterator itr = omd->c_values_for_send_by_ip.begin();
  std::map<std::string, vector<struct c_values> >::iterator itrEnd;

  for (int i = 0; i < thread_num; i++) {
    arr_ip[i] = itr->first;
    itr++;
  }
  for (int i = 0; i < thread_num; i++) {
    thread_arg[i].ip = arr_ip[i];
    thread_arg[i].max_ip_num = max_ip_num;
    thread_arg[i].next_ip_itr = &itr;
    pthread_create(&thread_id[i], NULL, send_each_ip_on_memory, (void *)&thread_arg[i]); 
  }
  for (int i = 0; i < thread_num; i++) {
    pthread_join(thread_id[i], NULL);
  }
  std::cerr << "終わりん" << std::endl;
  return 0;
}
int UpdateDistance::setupUpdate2() {
  std::cout << "in setupUpdate2" << std::endl;
  char time_buff[] = "";
  time_t now = time(NULL);
  struct tm *pnow = localtime(&now);
  sprintf(time_buff, "%04d%02d%02d%02d%02d%02d", pnow->tm_year + 1900, pnow->tm_mon + 1, pnow->tm_mday, pnow->tm_hour, pnow->tm_min, pnow->tm_sec);
  std::string send_time = time_buff;
  ofstream fout("./send_start_time.csv", ios::app);
  fout << send_time << std::endl;
  fout.close();
  std::string query;
  std::string local_server_ip = LOCAL_SERVER_IP;
  // mysqlから隣接ノードテーブルを読み出し
  //query = "select distinct next_server_ip from neighbor_nodes where next_server_ip != \"" + local_server_ip + "\";";
  //query = "select distinct next_server_ip from neighbor_nodes where next_server_ip = \"10.58.58.2\"";
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
  //char thread_arg[ip_num][IP_SIZE];
  struct send_each_ip_arg thread_arg[PROPAGATION_THREAD_NUM];
  int max_ip_num = ip_num;
  int start_thread_num = PROPAGATION_THREAD_NUM;
  int thread_num;
  if (ip_num < PROPAGATION_THREAD_NUM) {
    thread_num = ip_num;
  } else {
    thread_num = PROPAGATION_THREAD_NUM;
  }
 
  for (i = 0; i < thread_num; i++) {
    //memcpy(thread_arg[i], (char *)ip[i], sizeof(char) * IP_SIZE);
    thread_arg[i].ip = (char *)ip[i];
    thread_arg[i].max_ip_num = max_ip_num;
    thread_arg[i].next_ip_num = &start_thread_num;
    thread_arg[i].ip_p = (char *)ip;
    pthread_create(&thread_id[i], NULL, send_each_ip, (void *)&thread_arg[i]); 
  }
  for (i = 0; i < thread_num; i++) {
    pthread_join(thread_id[i], NULL);
  }
  std::cerr << "終わりん" << std::endl;
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
void *send_each_ip_on_memory(void *arg) {
  std::string ip_s = (std::string)(((struct send_each_ip_on_memory_arg *)arg)->ip);
  int max_ip_num = (int)(((struct send_each_ip_on_memory_arg *)arg)->max_ip_num);
  std::map<std::string, vector<struct c_values> >::iterator *next_ip_itr = (std::map<std::string, vector<struct c_values> >::iterator *)(((struct send_each_ip_on_memory_arg *)arg)->next_ip_itr);
  int breakflag = 0;
  vector<std::string> skip_ip_vector;
  vector<std::string>::iterator skip_ip_vector_p = skip_ip_vector.begin();

  std::cout << "ip: " << ip_s << std::endl;
  std::cout << "max_ip_num: " << max_ip_num << std::endl;
  std::cout << "vector: " << (*next_ip_itr)->first << std::endl;
  while(breakflag != 1) {
    // デバッグ
    /*
    if (ip_s != "10.58.58.11") {
      return NULL;
    }
    */
    //ip_s = "10.58.58.2";
    //
    char s_head_buf[sizeof(struct message_header) + sizeof(int)];
    struct message_header *s_header = (struct message_header *)s_head_buf;
    setupMsgHeader(s_header, UPDATE_DISTANCE, 0, 0);
    int *column_num = (int *)((char *)s_header + sizeof(struct message_header));
    *column_num = omd->c_values_for_send_by_ip[ip_s].size();
    std::cout << "send column_num: " << *column_num << std::endl;
    TcpClient *tc;
    tc = new TcpClient();
    if (tc->InitClientSocket(ip_s.c_str(), "5566") == -1) {
    //if (tc->InitClientSocket("10.58.58.2", "5566") == -1) {
      std::cout << "send error" << std::endl;
    }
    tc->SendMsg((char *)s_head_buf, sizeof(struct message_header) + sizeof(int));
    // recv側の接続数を限定する
    char r_buf[sizeof(int)];
    tc->RecvMsgAll((char *)r_buf, sizeof(int));
    std::cout << "r_buf: " << (int)*r_buf << std::endl;
    if ((int)*r_buf == -1) {
      skip_ip_vector.push_back(ip_s);
      pthread_mutex_lock(&propagation_thread_mutex);
      if (*next_ip_itr == omd->c_values_for_send_by_ip.end()) {
        breakflag = 1;
        if (skip_ip_vector_p == skip_ip_vector.end()) {
          breakflag = 1;
        } else {
          ip_s = *skip_ip_vector_p;
        }
      } else {
        ip_s = (*next_ip_itr)->first;
        (*next_ip_itr)++;
      }
      pthread_mutex_unlock(&propagation_thread_mutex);
      std::cout << "next_ip: " << ip_s << std::endl;
      continue;
    }
    int sent_column_num = 0;
    while(sent_column_num < *column_num) {
      int send_column_num;
      
      if ((*column_num - sent_column_num) < MAX_COLUMN_NUM) {
        send_column_num = *column_num - sent_column_num;
      } else {
        send_column_num = MAX_COLUMN_NUM;
      }
      int buf_size = sizeof(struct c_values) * (send_column_num);
      char s_buf[buf_size];
      std::cout << "debug: " << *column_num << std::endl;
      omd->copyBufferCValuesForSendByIp((char *)s_buf, ip_s, sent_column_num, send_column_num);
      std::cout << "debug: " << send_column_num << std::endl;
      // buffにちゃんと入っているかテスト
      /*
      char *debug_s_buf_p = s_buf;
      for (int i = 0; i < send_column_num; i++) {
        struct c_values test_c;
        memcpy(&test_c, (char *)debug_s_buf_p, sizeof(struct c_values));
        std::cout << "own_content_id: " << (test_c).own_content_id << std::endl;
        std::cout << "oth_content_id: " << (test_c).other_content_id << std::endl;
        std::cout << "version_id:     " << (test_c).version_id << std::endl;
        std::cout << "hop       :     " << (test_c).hop << std::endl;
        std::cout << "next_value:     " << (test_c).next_value << std::endl;
        for (int i = 0; i < MAX_HOP; i++) {
          std::cout << "value_chain:    " << (test_c).value_chain[i] << std::endl;
        }
        for (int i = 0; i < MAX_HOP - 1; i++) {
          std::cout << "node_value:     " << (test_c).node_chain[i] << std::endl;
        }
        std::cout << "get_time  :     " << (test_c).get_time << std::endl;
        debug_s_buf_p = debug_s_buf_p + sizeof(struct c_values);
      }
      */
      tc->SendMsg((char *)s_buf, buf_size);
      sent_column_num += MAX_COLUMN_NUM;
    }

    pthread_mutex_lock(&propagation_thread_mutex);
    if (*next_ip_itr == omd->c_values_for_send_by_ip.end()) {
      if (skip_ip_vector_p == skip_ip_vector.end()) {
        breakflag = 1;
      } else {
        ip_s = *skip_ip_vector_p;
      }
    } else {
      ip_s = (*next_ip_itr)->first;
      (*next_ip_itr)++;
    }
    pthread_mutex_unlock(&propagation_thread_mutex);
    std::cout << "next_ip: " << ip_s << std::endl;
  }

  return NULL;
}
void *send_each_ip(void *arg) {
  char *ip;
  ip = (char *)(((struct send_each_ip_arg *)arg)->ip);
  int max_ip_num = (int)(((struct send_each_ip_arg *)arg)->max_ip_num);
  int *next_ip_p = (int *)(((struct send_each_ip_arg *)arg)->next_ip_num);
  char ip_p[max_ip_num][IP_SIZE];
  memcpy((char *)ip_p, (char *)(((struct send_each_ip_arg *)arg)->ip_p), sizeof(char) * max_ip_num * IP_SIZE);
  /*
  for (int i = 0; i < max_ip_num; i++) {
      std::cout << "test ip: " << ip_p[i]<< std::endl;
  }
  */
  std::string ip_s = ip;
  fflush(stdout);

  int breakflag = 0;
  while(breakflag != 1) {
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
      std::string query = "select * from c_values join neighbor_nodes on c_values.own_content_id=neighbor_nodes.own_content_id where next_server_ip=\"" + ip_s + "\" and c_values.hop=" + i_str + " and c_values.path_chain not like concat(\"\%\",c_values.other_content_id,\"\%\");";
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
    if (tc->InitClientSocket(ip_s.c_str(), "5566") == -1) {
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
            + (sizeof(char) * (CONTENT_ID_SIZE * (i + 1) + i))) 
          * cn;

        char s_buf[buf_size];
        struct neighbor_node_column *n_n_c_p = (struct neighbor_node_column *)s_buf;
        int j = 0;
        for (int c = 0; c < cn; c++) {
          row = mysql_fetch_row(thread_db[i].result);
          strcpy(n_n_c_p->own_content_id, row[1]);// start_node
          strcpy(n_n_c_p->other_content_id, row[9]);// next_node
          strcpy(n_n_c_p->version_id, row[2]);
          std::string n_val_str = row[4];
          double *s_n_v = (double *)((char *)n_n_c_p + sizeof(struct neighbor_node_column));
          *s_n_v = atof(row[4]);
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
        tc->SendMsg((char *)s_buf, buf_size);
        char *debug_p = (char *)s_buf;
        /*
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
           */
        std::cout << "100 end: " << ip_s << std::endl;
      }
    }
    std::cout << "end: " << ip_s << std::endl;
    pthread_mutex_lock(&propagation_thread_mutex);
    *next_ip_p = *next_ip_p + 1;
    std::cerr << "next_ip_p: " << *next_ip_p << std::endl;
    std::cerr << "max_ipn: " << max_ip_num << std::endl;
    if (*next_ip_p > max_ip_num) {
      breakflag = 1;
    } else {
      ip_s = ip_p[*next_ip_p];
    }
    std::cout << "next_ip: " << ip_s << std::endl;
    pthread_mutex_unlock(&propagation_thread_mutex);

  } 
  return NULL;
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
      uda->updateCvalueNeighborOnMemory();
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

