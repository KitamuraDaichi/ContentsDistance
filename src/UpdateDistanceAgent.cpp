#include <condis.h>
#include <UpdateDistanceAgent.h>

UpdateDistanceAgent::~UpdateDistanceAgent() {
  delete &db;
  //delete ts;
}
UpdateDistanceAgent::UpdateDistanceAgent(struct client_data cdata) {
  if (db.connectDb("localhost", "root", "hige@mos", "contents_distance") < 0) {
    fprintf(stderr, "Databaseにconnectできませんでした。\n");
  }
  ts = new Tcp_Server();
  ts->cdata = cdata;
}
void UpdateDistanceAgent::updateDistanceFromCs() {
  std::cout << "in updateDistanceFromCs" << std::endl;
  struct message_from_cs mess;

  this->ts->recvMsgAll((char *)&mess, sizeof(struct message_from_cs));
  std::cout << "mess.value: " << (double)mess.cfec_part_value << std::endl;
}
void UpdateDistanceAgent::updateDistanceFromGm() {
  std::cout << "in updateDistanceFromGm" << std::endl;
}

int UpdateDistanceAgent::distance(std::string own_id, std::string other_id) {
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
