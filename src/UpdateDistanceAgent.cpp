#include <condis.h>
#include <UpdateDistanceAgent.h>

UpdateDistanceAgent::~UpdateDistanceAgent() {
  delete &db;
  //delete ts;
}
UpdateDistanceAgent::UpdateDistanceAgent(struct client_data cdata) {
  rc = new ReadConfig("catalog_distance.conf");
  if (db.connectDb("localhost", "root", "", "cfec_database") < 0) {
    fprintf(stderr, "Databaseにconnectできませんでした。\n");
  }
  ts = new Tcp_Server();
  ts->cdata = cdata;
}
int UpdateDistanceAgent::updateDistanceFromCs() {
  std::cout << "in updateDistanceFromCs" << std::endl;
  struct message_to_neighbor_nodes mess;

  char time_buff[] = "";
  time_t now = time(NULL);
  struct tm *pnow = localtime(&now);
  sprintf(time_buff, "%04d%02d%02d%02d%02d", pnow->tm_year + 1900, pnow->tm_mon + 1, pnow->tm_mday, pnow->tm_hour, pnow->tm_min);
  std::string recv_time_stamp = time_buff;

  int column_num;
  this->ts->recvMsgAll((char *)&column_num, sizeof(int));
  std::cout << "column_num: " << column_num << std::endl;

  int hop;
  this->ts->recvMsgAll((char *)&hop, sizeof(int));
  std::cout << "hop: " << hop << std::endl;

  char arr_n_n_c[(sizeof(struct neighbor_node_column) + (sizeof(double) * hop) + (sizeof(char) * 34 * (hop - 1))) * column_num]; 
  char *arr_n_n_c_p = arr_n_n_c;
  for (int i = 0; i < column_num; i++) {
    struct neighbor_node_column *n_n_c = (struct neighbor_node_column *)arr_n_n_c_p;
    this->ts->recvMsgAll((char *)n_n_c, sizeof(struct neighbor_node_column));
    std::cout << "own_content_id: " << n_n_c->own_content_id << std::endl;
    std::cout << "other_content_id: " << n_n_c->other_content_id << std::endl;
    std::cout << "version_id: " << n_n_c->version_id << std::endl;

    double *value_chain = (double *)((char *)n_n_c + sizeof(struct neighbor_node_column));
    this->ts->recvMsgAll((char *)value_chain, sizeof(double) * hop);
    for (int j = 0; j < hop; j++) {
      std::cout << "value_chain: " << value_chain[j] << std::endl;
    }
    if (this->existColumn(n_n_c->other_content_id) < 0){
      std::cerr << "このコンテンツを所有していません。" << std::endl;
      return -1;
    }

    // データベース更新
    char *path_chain = (char *)((char *)value_chain + sizeof(double) * hop);
    if (hop == 1) {
      if (this->existSameRouteColumn(n_n_c->other_content_id, n_n_c->own_content_id, "NULL")) {
        this->deleteColumn(n_n_c->other_content_id, n_n_c->own_content_id, "NULL");
      }
      this->addDb(n_n_c->other_content_id, n_n_c->own_content_id, n_n_c->version_id, value_chain, hop, NULL, recv_time_stamp);
    } else if (hop > 1) {
      //char path_chain[(34 * hop) + (hop - 1)];
      this->ts->recvMsgAll((char *)path_chain, (sizeof(char) * 34) * (hop - 1));

      if (this->existSameRouteColumn(n_n_c->other_content_id, n_n_c->own_content_id, path_chain)) {
        this->deleteColumn(n_n_c->other_content_id, n_n_c->own_content_id, path_chain);
      }
      this->addDb(n_n_c->other_content_id, n_n_c->own_content_id, n_n_c->version_id, value_chain, hop, path_chain, recv_time_stamp);
    } else {
      std::cout << "ERROR" << std::endl;
    }
    // end データベース更新

    // 伝搬するパート
    // mysqlから隣接ノードテーブルを読み出し
    // nncpに隣接ノードテーブルを作成
    std::string query;
    std::string ownci = n_n_c->other_content_id;
    query = "select * from neighbor_nodes where own_content_id = \"" + ownci + "\";";
    int tmp;
    if ((tmp = this->db.sendQuery((char *)query.c_str())) < 0) {
      std::cerr << "sendQuery返り値: " << tmp << std::endl;
      return -1;
    }
    MYSQL_ROW row;
    int count = mysql_num_rows(this->db.result);
    struct neighbor_node_column nncp[count];
    int k = 0;
    while ((row = mysql_fetch_row(this->db.result))) {
      struct neighbor_node_column nnc;
      strcpy(nnc.own_content_id, row[0]);
      strcpy(nnc.other_content_id, row[1]);
      strcpy(nnc.version_id, row[2]);
      memcpy(&(nncp[k]), &nnc, sizeof(neighbor_node_column));
      k++;
    }
    // サーバ圧縮
    for (k = 0; k < count; k++) {
      std::string ip;
      std::string id_first = nncp[k].other_content_id;
      id_first = id_first.substr(0, 8);
      rc->getParam(id_first, &ip);
      struct message_and_next_content_id tmp_manci;
      tmp_manci.n_n_c_p = (char *)arr_n_n_c_p;
      memcpy(tmp_manci.next_content_id, (char *)nncp[k].other_content_id, sizeof(char) * 34);
      tmp_manci.degree = count;
      this->dest_compless_map[ip].push_back(tmp_manci);
      //std::cout << "value_chain" << *(double *)((char *)tmp_manci.n_n_c_p + sizeof(struct neighbor_node_column)) << std::endl;
    }
    // end サーバ圧縮

    // end mysqlから隣接ノードテーブルを読み出し
    // end nncpに隣接ノードテーブルを作成
    arr_n_n_c_p = (char *)((char *)path_chain + (sizeof(char) * 34) * (hop - 1));
  }
  struct neighbor_node_column *start_p = (struct neighbor_node_column *)arr_n_n_c;
  for (int l = 0; l < column_num; l++) {
    std::cout << "s_own_id: " << start_p->own_content_id << std::endl;
    std::cout << "s_oth_id: " << start_p->other_content_id << std::endl;
    std::cout << "s_ver_id: " << start_p->version_id << std::endl;
    start_p = (struct neighbor_node_column *)((char *)start_p + sizeof(struct neighbor_node_column) + sizeof(double));
  }

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

  std::map<std::string, vector<struct message_and_next_content_id> >::iterator it;
  std::map<std::string, vector<struct message_and_next_content_id> >::iterator itEnd 
    = dest_compless_map.end();
  for (it = dest_compless_map.begin(); it != itEnd; it++) {
    char s_buf[sizeof(struct message_header) + sizeof(int) + sizeof(int)
      + (sizeof(neighbor_node_column) + (sizeof(double) * (hop + 1)) + (sizeof(char) * 34 * (hop)))
      * it->second.size()];
    struct message_header *next_header = (struct message_header *)s_buf;
    setupMsgHeader(next_header, UPDATE_DISTANCE, 0, 0);
    int *next_column_num = (int *)((char *)next_header + sizeof(struct message_header));
    *next_column_num = it->second.size();
    int *next_hop = (int *)((char *)next_column_num + sizeof(int));
    *next_hop = hop + 1;
    struct neighbor_node_column *next_n_n_c = (struct neighbor_node_column *)((char *)next_hop + sizeof(int));

    std::cout << "ip: " << it->first << std::endl;
    std::vector<struct message_and_next_content_id>::iterator v_itr;
    std::vector<struct message_and_next_content_id>::iterator v_itrEnd = (it->second).end();
    for (v_itr = (it->second).begin(); v_itr != v_itrEnd; v_itr++) {
      memcpy(&(next_n_n_c->own_content_id), (char *)((*v_itr).n_n_c_p), sizeof(char) * 34);
      std::cout << "own_ci: " << next_n_n_c->own_content_id << std::endl;
      memcpy(&(next_n_n_c->other_content_id), (char *)((*v_itr).next_content_id), sizeof(char) * 34);
      std::cout << "oth_ci: " << next_n_n_c->other_content_id << std::endl;
      memcpy(&(next_n_n_c->version_id), (char *)((*v_itr).n_n_c_p) + sizeof(char) * 34 * 2, sizeof(char) * 26);
      std::cout << "ver_ci: " << next_n_n_c->version_id << std::endl;
      double *next_value_chain = (double *)((char *)next_n_n_c + sizeof(struct neighbor_node_column));
      memcpy(next_value_chain, (char *)((*v_itr).n_n_c_p) + sizeof(struct neighbor_node_column), sizeof(double) * hop);
      if ((*v_itr).degree == 0) {
        std::cout << "degree error" << std::endl;
      }
      //std::cout << "value_chain: " << *((double *)(((*v_itr).n_n_c_p) + sizeof(struct neighbor_node_column))) << std::endl;
      next_value_chain[hop] = (double)next_value_chain[hop - 1] / (double)((*v_itr).degree);
      std::cout << "value_chain1: " << next_value_chain[hop - 1] << std::endl;
      std::cout << "value_chain2: " << next_value_chain[hop - 1] / (double)((*v_itr).degree) << std::endl;
      /*
      for (int t = 0; t < hop + 1; t++) {
        std::cout << "value_chain: " << next_value_chain[t] << std::endl;
      }
      */
      char *next_node_chain = (char *)((char *)next_value_chain + sizeof(double) * (hop + 1));
      memcpy(next_node_chain, (char *)((*v_itr).n_n_c_p) + sizeof(struct neighbor_node_column) + sizeof(double) * hop, sizeof(char) * 34 * (hop - 1));
      memcpy(next_node_chain + sizeof(char) * 34 * (hop - 1), (char *)((*v_itr).n_n_c_p) + sizeof(char) * 34, sizeof(char) * 34);
      next_n_n_c = (struct neighbor_node_column *)((char *)next_node_chain + sizeof(char) * 34 * hop);
    }
    if (it->first == "10.58.58.4") {
      in_port_t gm_port;
      rc->getParam("CONTENT_DISTANCE_PORT", &gm_port);
      ostringstream os;
      os << gm_port;
      std::string str_gm_port = os.str();
      TcpClient *tc2;
      tc2 = new TcpClient();
      if (tc2->InitClientSocket(it->first.c_str(), str_gm_port.c_str()) == -1) {
        std::cout << "send error" << std::endl;
      }
      std::cout << "debug 3" << std::endl;
      tc2->SendMsg((char *)s_buf, sizeof(s_buf));
      std::cout << "debug 5" << std::endl;
    }
    
    //std::cout << "next_header: " << *next_header << std::endl;
    /*
    std::cout << "next_column_num: " << *next_column_num << std::endl;
    std::cout << "next_hop: " << *next_hop << std::endl;
    next_n_n_c = (struct neighbor_node_column *)((char *)next_hop + sizeof(int));
    for (v_itr = (it->second).begin(); v_itr != v_itrEnd; ++v_itr) {
      std::cout << "own_id: " << next_n_n_c->own_content_id << std::endl;
      std::cout << "oth_id: " << next_n_n_c->other_content_id << std::endl;
      std::cout << "ver_id: " << next_n_n_c->version_id << std::endl;
      double *next_value_chain = (double *)((char *)next_n_n_c + sizeof(struct neighbor_node_column));
      for (int i = 0; i < hop + 1; i++) {
        std::cout << "value_chain: " << next_value_chain[i] << std::endl;
      }
      char *next_node_chain = (char *)((char *)next_value_chain + sizeof(double) * (hop + 1));
      std::cout << "next_node_chain: " << next_node_chain << std::endl;
      next_n_n_c = (struct neighbor_node_column *)((char *)next_node_chain + sizeof(char) * 34 * hop);
    }
    */
  }
  return 1; 
}
void UpdateDistanceAgent::updateDistanceFromGm() {
  std::cout << "in updateDistanceFromGm" << std::endl;
}

int UpdateDistanceAgent::addDb(char *own_content_id, char *other_content_id, char *version_id, double *value_chain, int hop, char *path_chain, std::string recv_time_stamp) {
  std::string query;
  std::string ownci = own_content_id;
  std::string othci = other_content_id;
  std::string verci = version_id;
  std::string valch;
  ostringstream os;

  for (int i = 0; i < hop; i++) {
    os << value_chain[i] << ",";
  }
  valch = os.str();

  if (path_chain == NULL) {
    query = "insert into c_values (own_content_id, other_content_id, version_id, value_chain, path_chain, recv_time_stamp) values(\""
      + ownci + "\", \"" + othci + "\", \"" + verci + "\", \"" + valch + "\", \"NULL\", \"" + recv_time_stamp + "\");";
  } else {
    std::string pathc = path_chain;
    query = "insert into c_values (own_content_id, other_content_id, version_id, value_chain, path_chain, recv_time_stamp) values(\""
      + ownci + "\", \"" + othci + "\", \"" + verci + "\", \"" + valch + "\", \"" + pathc + "\", \"" + recv_time_stamp + "\");";
  }

  std::cout << query << std::endl;
  int tmp;
  if ((tmp = this->db.sendQuery((char *)query.c_str())) < 0) {
    std::cerr << "sendQuery返り値: " << tmp << std::endl;
    return -1;
  }
  char **result = this->db.getResult();
  if (result != NULL) {
    std::cout << result[0] << std::endl;
  }

  return 0;
}

int UpdateDistanceAgent::propagateUpdate(){
  return 0;
}

int UpdateDistanceAgent::calculateDistances(struct node_id other_content_id){
  std::string query;
  std::string othci = id_to_string(other_content_id);
  int i;

  query = "select * from own_contents where other_content_id = \"" + othci + "and hop = 1\" ;";
  if (this->db.sendQuery((char *)query.c_str()) != 1) {
    return -1;
  }

  char **result = this->db.getResult();
  if (result != NULL) {
    return -1;
  } else {
    std::cout << result[0] << std::endl;
  }
  int result_size = sizeof(result) / sizeof(result[0]);

  //struct hash<std::string> hash_result;
  for(i = 0; i < result_size; i++) {
     
  }

  return 0;
}

int UpdateDistanceAgent::existColumn(char *content_id) {
  std::string own_content_id = content_id;
  std::string query;

  query = "select * from neighbor_nodes where own_content_id = \"" + own_content_id + "\" ;";

  int res = this->db.sendQuery((char *)query.c_str());
  if (res < 1) {
    std::cout << "res: " << res << std::endl;
    std::cout << "query: " << query << std::endl;

    return -1;
  }

  char **result = this->db.getResult();
  std::cout << result[0] << std::endl;
  return 1;
}

int UpdateDistanceAgent::existSameRouteColumn(char *own_content_id, char *other_content_id, char *path_chain) {
  std::string ownci = own_content_id;
  std::string othci = other_content_id;
  std::string pathc = path_chain;
  std::string query;

  query = "select * from c_values where own_content_id = \"" + ownci + "\" and other_content_id = \"" + othci + "\" and path_chain = \"" + pathc + "\";";

  if (this->db.sendQuery((char *)query.c_str()) < 0) {
    return -1;
  }

  if (this->db.getRowNum() > 0) {
    return 1;
  } else {
    return 0;
  }
  return 0;
}

int UpdateDistanceAgent::deleteColumn(char *own_content_id, char *other_content_id, char *path_chain) {
  std::string ownci = own_content_id;
  std::string othci = other_content_id;
  std::string pathc = path_chain;
  std::string query;

  query = "delete from c_values where own_content_id = \"" + ownci + "\" and other_content_id = \"" + othci + "\" and path_chain = \"" + pathc + "\";";

  if (this->db.sendQuery((char *)query.c_str()) < 0) {
    return -1;
  }

  return 1;
}

int UpdateDistanceAgent::distance(std::string own_id, std::string other_id) {
  std::string query;
  query = "select * from own_contents where own_content_id = \"" + own_id + "\" ;";

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

std::string id_to_string(struct node_id id) {
  std::string s;
  std::ostringstream sout;
  
  sout << std::setfill('0') << std::setw(8) << std::hex << id.first;
  sout << std::setfill('0') << std::setw(8) << std::hex << id.second;
  sout << std::setfill('0') << std::setw(8) << std::hex << id.third;
  sout << std::setfill('0') << std::setw(8) << std::hex << id.fourth;
  s = sout.str();

  return s;
}
std::string int_to_string(int num) {
  ostringstream s;
  s << num;
  
  return s.str();
}
std::string double_to_string(double num) {
  ostringstream s;
  s << num;
  
  return s.str();
}

void *propagate_thread(void *arg) {
  TcpClient *tc;
  std::string str_buf;
  struct message_header smsg_h;
  struct message_second smsg_s;
  int agent_num = ((struct propagate_thread_arg *)arg)->agent_num;
  int thread_num = ((struct propagate_thread_arg *)arg)->thread_num;

  //memcpy(thread_arg, additional_arg, arg_size);
  setupMsgHeader(&smsg_h, UPDATE_DISTANCE_SECOND, 0, 0);
  smsg_h.convert_hton();

  /*
  memcpy(*smsg_s, *(arg->message_second), size(struct message_second));
  smsg_c.source_id = node_id(0x0000000c, 0x0000000c, 0x0000000c, 0x0000000c);
  smsg_c.dest_id = node_id(0x0000000a, 0x0000000a, 0x0000000a, 0x0000000a);
  smsg_c.hop = 1;
  smsg_c.cfec_part_value = 100;

  str_buf.append((char *)&smsg_h, sizeof(struct message_header));
  str_buf.append((char *)&smsg_c, sizeof(struct message_from_cs));

  tc = new TcpClient();

  tc->SendMsg((char *)str_buf.c_str(), str_buf.size());
  */
}
