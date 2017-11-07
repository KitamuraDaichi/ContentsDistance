#include <condis.h>
#include <UpdateDistanceAgent.h>

UpdateDistanceAgent::~UpdateDistanceAgent() {
  delete &db;
  //delete ts;
}
UpdateDistanceAgent::UpdateDistanceAgent(struct client_data cdata) {
  rc = new ReadConfig("catalog_distance.conf");
  if (db.connectDb("localhost", "root", "hige@mos", "contents_distance") < 0) {
    fprintf(stderr, "Databaseにconnectできませんでした。\n");
  }
  ts = new Tcp_Server();
  ts->cdata = cdata;
}
int UpdateDistanceAgent::updateDistanceFromCs() {
  std::cout << "in updateDistanceFromCs" << std::endl;
  struct neighbor_node_column n_n_c;
  //n_n_c = (struct neighbor_node_column *)malloc(sizeof(struct neighbor_node_column));
  struct message_to_neighbor_nodes mess;
  this->ts->recvMsgAll((char *)&n_n_c, sizeof(struct neighbor_node_column));
  std::cout << "own_content_id: " << n_n_c.own_content_id << std::endl;
  std::cout << "other_content_id: " << n_n_c.other_content_id << std::endl;
  std::cout << "version_id: " << n_n_c.version_id << std::endl;

  int hop;
  this->ts->recvMsgAll((char *)&hop, sizeof(int));
  std::cout << "hop: " << hop << std::endl;
  double value_chain[hop];
  this->ts->recvMsgAll((char *)value_chain, sizeof(double) * hop);
  std::cout << "value_chain: " << value_chain[0] << std::endl;
  return 0;
  this->ts->recvMsgAll((char *)&mess, sizeof(struct message_to_neighbor_nodes));

  std::cout << "start_content_id: " << mess.start_content_id << std::endl;
  std::cout << "next_content_id: " << mess.next_content_id << std::endl;
  std::cout << "version_id: " << mess.version_id << std::endl;
  std::cout << "hop: " << mess.hop << std::endl;
  std::cout << "value_chain: " << mess.value_chain << std::endl;
  std::cout << "node_chain: " << mess.node_chain << std::endl;

  std::cout << "ここまで！" << std::endl;
  return 1;

  if (this->existColumn(mess.next_content_id) < 0){
    std::cerr << "このコンテンツを所有していません。" << std::endl;
    return -1;
  } else {
    std::cout << "このコンテンツを所有していました。" << std::endl;
    int tmp;
    if ((tmp = this->addDB(mess)) < 0) {
      std::cerr << "返り値: " << tmp << std::endl;
      std::cerr << "データベースに更新できませんでした。" << std::endl;
      return -1;
    } else {
      std::cout << "データベースを更新しました。" << std::endl;
    }
    /*
    if ((tmp = this->propagateUpdate(mess.dest_id)) < 0) {
    }
    */
  }
 return 1; 
}
void UpdateDistanceAgent::updateDistanceFromGm() {
  std::cout << "in updateDistanceFromGm" << std::endl;
}

int UpdateDistanceAgent::addDB(struct message_to_neighbor_nodes mess) {
  char buff[] = "";
  std::string time_stamp;

  time_t now = time(NULL);
  struct tm *pnow = localtime(&now);
  sprintf(buff, "%04d%02d%02d%02d%02d", pnow->tm_year + 1900, pnow->tm_mon + 1, pnow->tm_mday,
      pnow->tm_hour, pnow->tm_min);
  time_stamp = buff;

  std::string query;
  /*
  query = "insert into c_values(own_content_id, other_content_id, version_id, value_chain, path_chain, recv_time_stamp) values(\""
    + mess.next_content_id + "\", \"" + mess.start_content_id + "\", \"" + mess.version_id + "\", \"" + mess.value_chain + "\", \"" + mess.node_chain + "\", \"" + time_stamp + "\");";

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
  */

  return 0;
}

int UpdateDistanceAgent::propagateUpdate(struct node_id own_content_id){
  std::string query;
  std::string ownci = id_to_string(own_content_id);
  int i;
  //std::map<std::string, int> *map_itr;
  std::map<std::string, int>::iterator map_itr;

  query = "select * from distances where own_content_id = \"" + ownci + "\" and hop = 1 ;";
  int tmp;
  if ((tmp = this->db.sendQuery((char *)query.c_str())) < 0) {
    return -1;
  }

  char **result = this->db.getResult();
  if (result == NULL) {
    return -1;
  } else {
    std::cout << result[1] << std::endl;
  }
  int result_size = this->db.getRowNum();

  std::map<std::string, int> list_propagated;
  for(i = 0; i < result_size; i++) {
    list_propagated.insert(std::make_pair(result[1], 1));
    result = this->db.getResult();
  }
  for(map_itr = list_propagated.begin(); map_itr != list_propagated.end(); map_itr++) {
    in_port_t gm_port;
    rc->getParam("CONTENT_DISTANCE_PORT", &gm_port);
    std::string ip;
    std::string id_first = (map_itr->first).substr(0, 8);// キーへのアクセス
    rc->getParam(id_first, &ip);
    std::cout << "id_first: " << id_first;
    std::cout << "port: " << gm_port << std::endl;
    std::cout << "ip: " << ip << std::endl;

	  pthread_t thread_id;
    struct propagate_thread_arg thread_arg;
    char *pta = new char[sizeof(struct propagate_thread_arg)];

    pthread_create(&thread_id, NULL, propagate_thread, pta);


  }

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

int UpdateDistanceAgent::existColumn(std::string own_content_id) {
  std::string query;

  query = "select * from neighbor_nodes where own_content_id = \"" + own_content_id + "\" ;";

  if (this->db.sendQuery((char *)query.c_str()) != 1) {
    return -1;
  }

  char **result = this->db.getResult();
  std::cout << result[0] << std::endl;
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
