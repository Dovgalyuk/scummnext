#ifndef DEBUG_H
#define DEBUG_H

#define DEBUG_PUTC(c) debug_putc(c)
#define DEBUG_PUTS(s) debug_puts(s)
#define DEBUG_PRINTF(s, ...) debug_printf(s, __VA_ARGS__)
#define DEBUG_HALT while(1)

void debug_putc(char c);
void debug_puts(const char *s);
void debug_printf(const char *s, ...);

#endif
