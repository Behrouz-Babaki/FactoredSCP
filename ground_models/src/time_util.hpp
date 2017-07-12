#pragma once 

#include <time.h>
#include <sys/time.h>

/* from http://stackoverflow.com/a/17440673/7450070 */
inline double get_wall_time(){
  struct timeval time;
  if (gettimeofday(&time,NULL)){
    //  Handle error
    return 0;
  }
  return (double)time.tv_sec + (double)time.tv_usec * .000001;
}

inline double get_cpu_time(){
  return (double)clock() / CLOCKS_PER_SEC;
}