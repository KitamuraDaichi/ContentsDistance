#include <iostream>
#include <stdio.h>

int main()
{
  char buff[] = "";

  time_t now = time(NULL);
  struct tm *pnow = localtime(&now);
  sprintf(buff, "%04d%02d%02d%02d%02d%02d", pnow->tm_year + 1900, pnow->tm_mon + 1, pnow->tm_mday,
      pnow->tm_hour, pnow->tm_min, pnow->tm_sec);

  std::cout << buff << std::endl;
}
