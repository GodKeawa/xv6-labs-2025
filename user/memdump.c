#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

void memdump(char *fmt, char *data);

int
main(int argc, char *argv[])
{
  if(argc == 1){
    printf("Example 1:\n");
    int a[2] = { 61810, 2025 };
    memdump("ii", (char*) a);
    
    printf("Example 2:\n");
    memdump("S", "a string");
    
    printf("Example 3:\n");
    char *s = "another";
    memdump("s", (char *) &s);

    struct sss {
      char *ptr;
      int num1;
      short num2;
      char byte;
      char bytes[8];
    } example;
    
    example.ptr = "hello";
    example.num1 = 1819438967;
    example.num2 = 100;
    example.byte = 'z';
    strcpy(example.bytes, "xyzzy");
    
    printf("Example 4:\n");
    memdump("pihcS", (char*) &example);
    
    printf("Example 5:\n");
    memdump("sccccc", (char*) &example);
  } else if(argc == 2){
    // format in argv[1], up to 512 bytes of data from standard input.
    char data[512];
    int n = 0;
    memset(data, '\0', sizeof(data));
    while(n < sizeof(data)){
      int nn = read(0, data + n, sizeof(data) - n);
      if(nn <= 0)
        break;
      n += nn;
    }
    memdump(argv[1], data);
  } else {
    printf("Usage: memdump [format]\n");
    exit(1);
  }
  exit(0);
}

static char digits[] = "0123456789ABCDEF";
void printLX(uint64 num) {
  char hex_str[16]; // 最长的情况下是16位
  // 处理小端序
  for (int i = 15; i >= 0; i--) {
    hex_str[15-i] = digits[(num >> (i * 4)) & 0xF];
  }
  // 处理前导0
  int length = 16;
  for (int i = 0; hex_str[i] == '0'; i++) length--;
  write(1, hex_str + 16 - length, length);
}

void
memdump(char *fmt, char *data)
{
  // Your code here.
  // 已知printf 当前的p模式实现有问题，因此只需要做一个fix即可
  // 对于非S的模式都有固定的长度，因此可以逐个输出
  int ptr = 0;
  for (int i = 0; fmt[i];  i++) {
    switch (fmt[i]) {
      case 'i': {
        int tmp = 0;
        memcpy(&tmp, data + ptr, 4);
        printf("%d\n", tmp);
        ptr += 4;
        break;
      }
      case 'p': {
        uint64 tmp = 0;
        memcpy(&tmp, data + ptr, 8);
        printLX(tmp);
        printf("\n");
        ptr += 8;
        break;
      }
      case 'h': {
        short tmp = 0;
        memcpy(&tmp, data + ptr, 2);
        printf("%d\n", tmp);
        ptr += 2;
        break;
      }
      case 'c': {
        char tmp = *(data + ptr);
        printf("%c\n", tmp);
        ptr += 1;
        break;
      }
      case 's': {
        uint64 tmp_ptr = 0;
        memcpy(&tmp_ptr, data + ptr, 8);
        char *tmp = (char*)tmp_ptr;
        printf("%s\n", tmp);
        ptr += 8;
        break;
      }
      case 'S': {
        printf("%s\n", data + ptr);
        break;
      }
    }
    if (fmt[i] == 'S') break;
  }
}
