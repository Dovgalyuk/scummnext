#include <stdarg.h>
#include <stdint.h>
#include "debug.h"

__sfr __banked __at 0xCF3B DEBUG_PORT;
__sfr __banked __at 0xDF3B DEBUG_DATA;

void debug_putc(char c)
{
    DEBUG_PORT = 1;
    DEBUG_DATA = c;
}

void debug_puts(const char *s)
{
    while (*s)
        debug_putc(*s++);
}

static void debug_print_num(uint32_t num, uint8_t base)
{
    char buf[12];
    int8_t i = 0;
    while (num)
    {
        char c = num % base + '0';
        if (c > '9')
            c += 'a' - '0' - 10;
        buf[i++] = c;
        num /= base;
    }
    if (i == 0)
    {
        debug_putc('0');
    }
    else
    {
        while (--i >= 0)
            debug_putc(buf[i]);
    }
}

void debug_printf(const char *s, ...)
{
    va_list ap;
    va_start(ap, s);

    while (*s)
    {
        char c = *s++;
        if (c == '%')
        {
            switch (*s++)
            {
            case '%':
                debug_putc('%');
                break;
            case 'u':
                {
                    uint16_t arg = va_arg(ap, uint16_t);
                    debug_print_num(arg, 10);
                }
                break;
            case 'x':
                {
                    uint16_t arg = va_arg(ap, uint16_t);
                    debug_print_num(arg, 16);
                }
                break;
            case 'd':
                {
                    int16_t arg = va_arg(ap, int16_t);
                    if (arg < 0)
                    {
                        debug_putc('-');
                        arg = -arg;
                    }
                    debug_print_num(arg, 10);
                }
                break;
            case 'l':
                {
                    int32_t arg = va_arg(ap, int32_t);
                    if (arg < 0)
                    {
                        debug_putc('-');
                        arg = -arg;
                    }
                    debug_print_num(arg, 10);
                }
                break;
            case 'c':
                debug_putc(va_arg(ap, int8_t));
                break;
            case 's':
                debug_puts(va_arg(ap, char*));
                break;
            }
        }
        else if (c != '\r')
        {
            debug_putc(c);
        }
    }
    va_end(ap);
}

uint8_t debug_data;
void debug_delay(unsigned int n)
{
    while (n--)
        ++debug_data;
}
