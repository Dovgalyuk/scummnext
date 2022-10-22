#ifndef DEBUG_H
#define DEBUG_H

//#define DEBUG
//#define HABR

#ifdef DEBUG
#define DEBUG_PUTC(c) debug_putc(c)
#define DEBUG_PUTS(s) debug_puts(s)
#define DEBUG_PRINTF(s, ...) debug_printf(s, __VA_ARGS__)
#define DEBUG_HALT while(1)
#define DEBUG_ASSERT(c, s) do { if (!(c)) { DEBUG_PRINTF("ASSERT %s\n", s); DEBUG_HALT; } } while (0)
#define DEBUG_DELAY(n) debug_delay(n)
#else
#define DEBUG_PUTC(c) while(0)
#define DEBUG_PUTS(s) while(0)
#define DEBUG_PRINTF(s, ...) while(0)
#define DEBUG_HALT while(1)
#define DEBUG_ASSERT(c, s) while(0)
#define DEBUG_DELAY(n) while(0)
#endif

void debug_putc(char c);
void debug_puts(const char *s);
void debug_printf(const char *s, ...);
void debug_delay(unsigned int n);

#endif
