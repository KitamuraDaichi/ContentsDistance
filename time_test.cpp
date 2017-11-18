#include <iostream>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <map>
#include <typeinfo>
#include <mysql/mysql.h>

int main()
{
  std::map<std::string, int> mp;
  mp["aa"] = 1;
  mp["bb"] = 2;
  mp["cc"] = 3;

  std::map<std::string, int>::iterator itEnd = mp.end();
  for (std::map<std::string, int>::iterator it = mp.begin(); it != itEnd; ++it) {
    std::cout << it->first << ": " << it->second << std::endl;
  }
  
/*
  double d = 1000000.3;
  std::ostringstream os;
  os << d;
  std::string d_str = os.str();
  char *d_c = (char *)d_str.c_str();
  int d_str_len = d_str.length();
  std::cout << d_str << std::endl;
  std::cout << d_str_len << std::endl;
  std::cout << d_c << std::endl;
  std::cout << sizeof(d_c) << std::endl;


  char *buf = "12345678910";
  std::cout << buf << std::endl;
  std::cout << sizeof(buf) << std::endl;
  

  char buff[] = "";

  time_t now = time(NULL);
  struct tm *pnow = localtime(&now);
  sprintf(buff, "%04d%02d%02d%02d%02d%02d", pnow->tm_year + 1900, pnow->tm_mon + 1, pnow->tm_mday,
      pnow->tm_hour, pnow->tm_min, pnow->tm_sec);

  std::cout << buff << std::endl;
  */
}
