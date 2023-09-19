#include "common.h"

//将整型转换成字符(integer to ascii)
static void itoa(unsigned int value, char **buf_ptr_addr, int base)
{
    unsigned int m = value % base;
    unsigned int i = value / base;
    if (i)
    {
        itoa(i, buf_ptr_addr, base);
    }
    if (m < 10)
    {
        *((*buf_ptr_addr)++) = m + '0';
    }
    else
    {
        *((*buf_ptr_addr)++) = m - 10 + 'A';
    }
}

//将参数ap按照格式format输出到字符串str,并返回替换后str长度
unsigned int vsprintf(char *str, const char *format, va_list ap)
{
    char *buf_ptr = str;
    const char *index_ptr = format;
    char index_char = *index_ptr;
    int arg_int;
    char *arg_str;
    while (index_char)
    {
        if (index_char != '%')
        {
            *(buf_ptr++) = index_char;
            index_char = *(++index_ptr);
            continue;
        }
        index_char = *(++index_ptr); //得到%后面的字符
        switch (index_char)
        {
        case 'x':
            arg_int = va_arg(ap, int);
            itoa(arg_int, &buf_ptr, 16);
            index_char = *(++index_ptr);
            break;
        case 's':
            arg_str = va_arg(ap, char *);
            strcpy(buf_ptr, arg_str);
            buf_ptr += strlen(arg_str);
            index_char = *(++index_ptr);
            break;
        case 'c':
            *(buf_ptr++) = va_arg(ap, char);
            index_char = *(++index_ptr);
            break;
        case 'd':
            arg_int = va_arg(ap, int);
            //若是负数，将其转为整数后，在正数前面输出个负号‘-’
            if (arg_int < 0)
            {
                arg_int = 0 - arg_int;
                *buf_ptr++ = '-';
            }
            itoa(arg_int, &buf_ptr, 10);
            index_char = *(++index_ptr);
            break;
        }
    }
    *buf_ptr = 0;
    return strlen(str);
}