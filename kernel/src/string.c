#include "common.h"

/*将dst_起始的size个字节置为value*/
void memset(void *dst_, unsigned char value, unsigned int size)
{
  unsigned char *dst = (unsigned char *)dst_;
  while (size-- > 0)
    *dst++ = value;
}

/*将src_起始的size个字节复制到dst_*/
void memcpy(void *dst_, const void *src_, unsigned int size)
{
  unsigned char *dst = dst_;
  const unsigned char *src = src_;
  while (size-- > 0)
    *dst++ = *src++;
}

/*连续比较以地址a_和地址b_开头的size个字节，若相等则返回0，若a_大于b_，返回+1，否则返回-1*/
int memcmp(const void *a_, const void *b_, unsigned int size)
{
  const char *a = a_;
  const char *b = b_;
  while (size-- > 0)
  {
    if (*a != *b)
    {
      return *a > *b ? 1 : -1;
    }
    a++;
    b++;
  }
  return 0;
}

/*将字符串从src_复制到dst_*/
char *strcpy(char *dst_, const char *src_)
{
  char *r = dst_; //用来返回目的字符串起始地址
  while ((*dst_++ = *src_++))
    ; //字符串以\0结尾
  return r;
}

/*返回字符串长度*/
unsigned int strlen(const char *str)
{
  const char *p = str;
  while (*p++)
    ;
  return (p - str - 1); //为啥书中要多减个1呢？因为此时的指针p指向了字符串结尾0后的那个字节。
}