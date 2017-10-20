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
  struct message_from_cs mess;
  struct node_id other_id = mess.source_id;
  struct node_id own_id = mess.dest_id;

  this->ts->recvMsgAll((char *)&mess, sizeof(struct message_from_cs));
  std::cout << "mess.value: " << (double)mess.cfec_part_value << std::endl;
  std::cout << "mess.own_id: " << id_to_string(mess.source_id) << std::endl;
  std::cout << "mess.own_id: " << mess.source_id.first << std::endl;
  std::cout << "mess.own_id: " << own_id.first << std::endl;
  std::cout << "mess.other_id: " << id_to_string(mess.dest_id) << std::endl;
  std::cout << "mess.other_id: " << mess.dest_id.first << std::endl;
  std::cout << "mess.other_id: " << other_id.first << std::endl;

  if (this->existColumn(mess.dest_id) < 0){
    std::cerr << "このコンテンツを所有していません。" << std::endl;
    std::cerr << "contents id: " << id_to_string(mess.dest_id) << std::endl;
    return -1;
  } else {
    std::cout << "このコンテンツを所有していました。" << std::endl;
    std::cerr << "contents id: " << id_to_string(mess.dest_id) << std::endl;
    int tmp;
    if ((tmp = this->addDB(mess.dest_id, mess.source_id, mess.hop, mess.cfec_part_value)) < 0) {
      std::cerr << "返り値: " << tmp << std::endl;
      std::cerr << "データベースに更新できませんでした。" << std::endl;
      std::cerr << "own id: " << id_to_string(mess.dest_id) << std::endl;
      std::cerr << "other id: " << id_to_string(mess.source_id) << std::endl;
      std::cerr << "hop: " << int_to_string(mess.hop) << std::endl;
      std::cerr << "value: " << double_to_string(mess.cfec_part_value) << std::endl;
      return -1;
    } else {
      std::cout << "データベースを更新しました。" << std::endl;
    }

    if ((tmp = this->propagateUpdate(mess.dest_id)) < 0) {
    }
    
    /*
    if ((tmp = this->calculateDistances(mess.source_id)) < 0) {

    }
    */

  }
 return 1; 
}
void UpdateDistanceAgent::updateDistanceFromGm() {
  std::cout << "in updateDistanceFromGm" << std::endl;
}

int UpdateDistanceAgent::addDB(struct node_id own_content_id, struct node_id other_content_id, int hop, double value) {
  std::string query;
  std::string ownci = id_to_string(own_content_id);
  std::string othci = id_to_string(other_content_id);

  query = "insert into distances(own_content_id, other_content_id, hop, distance) values(\""
    + ownci + "\", \"" + othci + "\", " + int_to_string(hop) + ", " + double_to_string(value) + ");";
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

int UpdateDistanceAgent::propagateUpdate(struct node_id own_content_id){
  std::string query;
  std::string ownci = id_to_string(own_content_id);
  int i;

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

  std::map<std::string, int> list_propageted;
  for(i = 0; i < result_size; i++) {
    list_propageted.insert(std::make_pair(result[1], 1));
    result = this->db.getResult();
  }
  std::map<std::string, int> id_ip;
  in_port_t gm_port;
  rc->getParam("CONTENT_DISTANCE_PORT", &gm_port);
  std::string ip;
  rc->getParam("0000000c", &ip);
  std::cout << "port: " << gm_port << std::endl;
  std::cout << "ip: " << ip << std::endl;

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

int UpdateDistanceAgent::existColumn(struct node_id own_content_id) {
  std::string query;
  std::string ownci = id_to_string(own_content_id);

  query = "select * from own_contents where own_content_id = \"" + ownci + "\" ;";

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
