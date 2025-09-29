#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char* argv[]) {
  // 第一个参数是名字，从第二个开始是参数
  if (argc != 2) { 
    if (argc == 1) {
      fprintf(1, "sleep: please set sleep time\n"); // 处理没有参数的情况
      exit(0);
    } else {
      fprintf(1, "sleep: Only one argument suggested\n"); // 只是通知一下
    }
  }
  // argc == 2
  int time = atoi(argv[1]);
  if (time <= 0) {
    fprintf(1, "sleep: bad time\n"); // 无效时间
    exit(-1);
  }
  fprintf(1, "(nothing happens for a little while)\n");
  pause(time); // system call
  exit(0);
}
