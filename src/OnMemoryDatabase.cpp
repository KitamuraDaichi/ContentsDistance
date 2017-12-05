#include <OnMemoryDatabase.h>
using namespace boost;
shared_mutex neighbor_nodes_mutex;
shared_mutex ip_mutex;
shared_mutex c_values_mutex;
shared_mutex mysql_mutex;

OnMemoryDatabase::~OnMemoryDatabase() {
}
OnMemoryDatabase::OnMemoryDatabase() {
  if (db.connectDb("localhost", "root", "", "cfec_database2") < 0) {
    fprintf(stderr, "Databaseにconnectできませんでした。\n");
  }
  this->loadMysql();
}
int OnMemoryDatabase::loadMysql() {
  if (this->loadIp() < -1) {
    std::cout << "loadIp Error" << std::endl;
  }
  if (this->loadCValuesForSendByIp() < -1) {
    std::cout << "loadCValuesForSendByIp Error" << std::endl;
  }
  /*
  if (this->loadNeighborNodes() < -1) {
    std::cout << "loadNeighborNode Error" << std::endl;
  }
  if (this->loadCValues() < -1) {
    std::cout << "loadCValues Error" << std::endl;
  }
  */

  return 1;
}
int OnMemoryDatabase::loadCValuesForSendByIp() {
  std::vector<std::string>::iterator v_itr;
  std::vector<std::string>::iterator v_itrEnd = next_ip_table.end();

  for (v_itr = next_ip_table.begin(); v_itr != v_itrEnd; ++v_itr) {
    std::string ip_s = *v_itr;
    ostringstream os;
    os << MAX_HOP - 1;// max_hopになっているのは次に伝えない MAX_HOP = 3     0 1 2   2になっているやつは次に伝えない
    std::string hop_s = os.str();

      std::string query = "select * from c_values join neighbor_nodes on c_values.own_content_id=neighbor_nodes.own_content_id where next_server_ip=\"" + ip_s + "\" and c_values.hop<" + hop_s + " and c_values.path_chain not like concat(\"\%\",c_values.other_content_id,\"\%\");";
    int tmp;
    if ((tmp = this->db.sendQuery((char *)query.c_str())) < 0) {
      std::cerr << "sendQuery返り値: " << tmp << std::endl;
      return NULL;
    }
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(this->db.result))) {
      struct c_values tmp_c_values;
      // 次のサーバにそのまま入れれる形にしてあげる感じ
      strcpy(tmp_c_values.own_content_id, row[9]); // dest_id
      strcpy(tmp_c_values.other_content_id, row[1]); // start_id
      strcpy(tmp_c_values.version_id, row[2]); 
      tmp_c_values.hop = atoi(row[3]) + 1;
      tmp_c_values.next_value = atof(row[4]);
      // value_chain
      char *tok;
      tok = strtok(row[5], ",");
      int i = 0;
      while (tok != NULL) {
        tmp_c_values.value_chain[i] = atof(tok);
        tok = strtok(NULL, ",");
        i++;
      }
      for (int j = i; j < MAX_HOP; j++) {
        tmp_c_values.value_chain[j] = -1.0;
      }
      // node_chain
      if (strcmp(row[6], "NULL") == 0) {
        strcpy(tmp_c_values.node_chain[0], row[0]); // 自分のコンテンツを末尾に入れておく
        for (int j = 1; j < MAX_HOP - 1; j++) {
          strcpy(tmp_c_values.node_chain[j],"NULL");
        }
      } else {
        char *tok_node;
        i = 0;
        strcpy(tmp_c_values.node_chain[i], row[0]); // 自分のコンテンツを末尾に入れておく
        i++;
        tok_node = strtok(row[6], ",");
        while (tok_node != NULL) {
          strcpy(tmp_c_values.node_chain[i], tok_node);
          tok_node = strtok(NULL, ",");
          i++;
        }
        for (int j = i; j < MAX_HOP - 1; j++) {
          strcpy(tmp_c_values.node_chain[j], "NULL");
        }
      }
      strcpy(tmp_c_values.get_time, row[7]); 
      // ipに引っ掛けるmap作成（送信時にそのまま使用）
      c_values_for_send_by_ip[ip_s].push_back(tmp_c_values);
    }
  }
}
int OnMemoryDatabase::loadNeighborNodes() {
  std::string query = "select * from neighbor_nodes;";
  int tmp;
  if ((tmp = this->db.sendQuery((char *)query.c_str())) < 0) {
    std::cerr << "sendQuery返り値: " << tmp << std::endl;
    return -1;
  }   
  MYSQL_ROW row;
  while ((row = mysql_fetch_row(this->db.result))) {
    struct neighbor_nodes tmp_neighbor_nodes;
    strcpy(tmp_neighbor_nodes.own_content_id, row[0]); 
    strcpy(tmp_neighbor_nodes.other_content_id, row[1]); 
    strcpy(tmp_neighbor_nodes.version_id, row[2]); 
    strcpy(tmp_neighbor_nodes.next_server_ip, row[3]); 
    // vectorに入れる
    neighbor_nodes_table.push_back(tmp_neighbor_nodes);
    // map作成
    std::string ip_s = tmp_neighbor_nodes.next_server_ip;
    neighbor_nodes_table_by_ip[ip_s].push_back(tmp_neighbor_nodes);
  }

  return 1;
}
int OnMemoryDatabase::loadIp() {
  std::string query = "select distinct next_server_ip from neighbor_nodes;";
  int tmp;
  if ((tmp = this->db.sendQuery((char *)query.c_str())) < 0) {
    std::cerr << "sendQuery返り値: " << tmp << std::endl;
    return -1;
  }   
  MYSQL_ROW row;
  while ((row = mysql_fetch_row(this->db.result))) {
    char tmp_ip[IP_SIZE];
    strcpy(tmp_ip, row[0]); 
    std::string ip_s = tmp_ip;
    this->next_ip_table.push_back(ip_s);
  }

  return 1;
}
int OnMemoryDatabase::loadCValues() {
  std::string query = "select * from c_values;";
  int tmp;
  if ((tmp = this->db.sendQuery((char *)query.c_str())) < 0) {
    std::cerr << "sendQuery返り値: " << tmp << std::endl;
    return -1;
  }   
  MYSQL_ROW row;
  while ((row = mysql_fetch_row(this->db.result))) {
    struct c_values tmp_c_values;
    strcpy(tmp_c_values.own_content_id, row[0]); 
    strcpy(tmp_c_values.other_content_id, row[1]); 
    strcpy(tmp_c_values.version_id, row[2]); 
    tmp_c_values.hop = atoi(row[3]);
    tmp_c_values.next_value = atof(row[4]);
    // value_chain
    char *tok;
    tok = strtok(row[5], ",");
    int i = 0;
    while (tok != NULL) {
      tmp_c_values.value_chain[i] = atof(tok);
      tok = strtok(NULL, ",");
      i++;
    }
    for (int j = i; j < MAX_HOP; j++) {
      tmp_c_values.value_chain[j] = 0.0;
    }
    // node_chain
    if (strcmp(row[6], "NULL") == 0) {
      for (int j = 0; j < MAX_HOP - 1; j++) {
        strcpy(tmp_c_values.node_chain[j],"NULL");
      }
    } else {
      char *tok_node;
      tok_node = strtok(row[6], ",");
      i = 0;
      while (tok_node != NULL) {
        strcpy(tmp_c_values.node_chain[i], tok_node);
        tok_node = strtok(NULL, ",");
        i++;
      }
      for (int j = i; j < MAX_HOP - 1; j++) {
        strcpy(tmp_c_values.node_chain[j], "NULL");
      }
    }
    strcpy(tmp_c_values.get_time, row[7]); 
    // vectorに入れる
    c_values_table.push_back(tmp_c_values);
    // map作成
    std::string own_content_id_s = tmp_c_values.own_content_id;
    c_values_by_own_content_id[own_content_id_s].push_back(tmp_c_values);
    // map作成
    std::string other_content_id_s = tmp_c_values.other_content_id;
    std::string node_chain_s = tmp_c_values.node_chain[0];
    for (int i = 1; i < MAX_HOP - 1; i++) {
      std::string tmp_node_id = tmp_c_values.node_chain[i];
      node_chain_s = node_chain_s + "," + tmp_node_id;
    }
    std::string start_path_end_s = own_content_id_s + node_chain_s + other_content_id_s;
    c_values_by_start_path_end[start_path_end_s] = tmp_c_values;
  }

  return 1;
}
int OnMemoryDatabase::testLoadNeighborNodes() {
  shared_lock <shared_mutex> read_lock(neighbor_nodes_mutex);
  neighbor_nodes_table;
  std::vector<struct neighbor_nodes>::iterator v_itr;
  std::vector<struct neighbor_nodes>::iterator v_itrEnd = neighbor_nodes_table.end();

  for (v_itr = neighbor_nodes_table.begin(); v_itr != v_itrEnd; ++v_itr) {
    std::cout << "own_content_id: " << (*v_itr).own_content_id << std::endl;
    std::cout << "oth_content_id: " << (*v_itr).other_content_id << std::endl;
    std::cout << "version_id:     " << (*v_itr).version_id << std::endl;
    std::cout << "next_server_ip: " << (*v_itr).next_server_ip << std::endl;
  }
}
int OnMemoryDatabase::testLoadIp() {
  boost::shared_lock <boost::shared_mutex> read_lock(ip_mutex);
  std::vector<std::string>::iterator v_itr;
  std::vector<std::string>::iterator v_itrEnd = next_ip_table.end();

  for (v_itr = next_ip_table.begin(); v_itr != v_itrEnd; ++v_itr) {
    std::cout << "ip            : " << *v_itr << std::endl;
  }
}
int OnMemoryDatabase::testLoadCValues() {
  boost::shared_lock <boost::shared_mutex> read_lock(c_values_mutex);
  std::vector<struct c_values>::iterator v_itr;
  std::vector<struct c_values>::iterator v_itrEnd = c_values_table.end();

  for (v_itr = c_values_table.begin(); v_itr != v_itrEnd; ++v_itr) {
    std::cout << "own_content_id: " << (*v_itr).own_content_id << std::endl;
    std::cout << "oth_content_id: " << (*v_itr).other_content_id << std::endl;
    std::cout << "version_id:     " << (*v_itr).version_id << std::endl;
    std::cout << "hop       :     " << (*v_itr).hop << std::endl;
    std::cout << "next_value:     " << (*v_itr).next_value << std::endl;
    for (int i = 0; i < MAX_HOP; i++) {
      std::cout << "value_chain:    " << (*v_itr).value_chain[i] << std::endl;
    }
    for (int i = 0; i < MAX_HOP - 1; i++) {
      std::cout << "node_value:     " << (*v_itr).node_chain[i] << std::endl;
    }
    std::cout << "get_time  :     " << (*v_itr).get_time << std::endl;
  }
}
int OnMemoryDatabase::writeNeighborNodes(struct neighbor_nodes new_column){
  upgrade_lock<shared_mutex> up_lock(neighbor_nodes_mutex);
  upgrade_to_unique_lock<shared_mutex> write_lock(up_lock);
  neighbor_nodes_table.push_back(new_column);
}
int OnMemoryDatabase::writeIp(std::string new_ip){
  upgrade_lock<shared_mutex> up_lock(ip_mutex);
  upgrade_to_unique_lock<shared_mutex> write_lock(up_lock);
  next_ip_table.push_back(new_ip);
}
int OnMemoryDatabase::writeCValues(struct c_values new_column){
  upgrade_lock<shared_mutex> up_lock(c_values_mutex);
  upgrade_to_unique_lock<shared_mutex> write_lock(up_lock);
  c_values_table.push_back(new_column);
}
int OnMemoryDatabase::copyBufferCValuesForSendByIp(char *s_buf, std::string ip, int start_num, int size) {
  std::vector<struct c_values>::iterator v_itr = c_values_for_send_by_ip[ip].begin() + start_num;
  std::vector<struct c_values>::iterator v_itrEnd = v_itr + size;

  char *s_buf_p = s_buf;
  for (; v_itr != v_itrEnd; ++v_itr) {
    memcpy(s_buf_p, (char *)&(*v_itr), sizeof(struct c_values));
    s_buf_p = s_buf_p + sizeof(struct c_values);
  }
  return 1;
}
