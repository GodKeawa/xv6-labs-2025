#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

// 因为文件大小未知，所以需要设置一个buffer
char buf[512];
int buf_ptr = 0;
// 保存一个数字的buffer, 由于int最大为2147483648, 为保证atoi工作,这里不处理太大的数
char num[16];
int num_ptr = 0;

void clearNum() {
  for (int i = 0; i < sizeof(num); i++) {
    num[i] = 0;
  }
}

int isSeparator(char c) {
  // \0不会被认为是separater
  return strchr("-\r\t\n./,", c) > 0;
}

int isNumber(char c) {
  return strchr("0123456789", c) > 0;
}

int validated() {
  for (int i = 0; i < num_ptr; i++) {
    if (!isNumber(num[i])) {
      return 0;
    }
  }
  return 1;
}

void printOne() {
  // 处理separater，如果处理时num_buffer里有东西，就验证并打印出来
  // 因为必须检测到separater才进行下面的逻辑，所以不会截断number
  while (buf_ptr < sizeof(buf) && isSeparator(buf[buf_ptr])) {
    buf_ptr++;
    if (num_ptr > 0) {
      if (validated()) { // 验证通过
        int number = atoi(num);
        if ((number % 5) == 0 || (number % 6) == 0) {
          fprintf(1, "%d\n", number);
        }
      }
      // 恢复num_buffer
      clearNum();
      num_ptr = 0;
    }
  } 
  // 继续填充num
  // 这里结束的\0也会被填入，但是行为没有问题
  while (buf_ptr < sizeof(buf) && (num_ptr < sizeof(num)) && !isSeparator(buf[buf_ptr])) {
    num[num_ptr] = buf[buf_ptr];
    buf_ptr++;
    num_ptr++;
  }
}


void sixfive(int fd) {
  int n;
  while((n = read(fd, buf, sizeof(buf))) > 0) {
    // 对一个buffer持续进行printOne
    buf_ptr = 0;
    while(buf_ptr < n) {
      printOne();
    }
  }
  if(n < 0){
    fprintf(2, "sixfive: read error\n");
    exit(1);
  }
}


int main(int argc, char* argv[]) {
  // 仿照cat的结构，支持多文件输入
  int fd, i;

  if(argc <= 1){
    fprintf(1, "sixfive: please input a file\n");
    exit(0);
  }

  for(i = 1; i < argc; i++){
    // 对每个文件单独处理
    if((fd = open(argv[i], O_RDONLY)) < 0){
      fprintf(2, "sixfive: cannot open %s\n", argv[i]);
      exit(1);
    }
    clearNum();
    num_ptr = 0;
    sixfive(fd);
    close(fd);
  }
  exit(0);
}
