#ifndef HELPER_H
#define HELPER_H

#define ABS(x) ((x) < 0 ? -(x) : (x))
#define MAX(a, b) ((a) < (b) ? (b) : (a))

#define PUSH_PAGE(p, v) \
    uint8_t page##p = ZXN_READ_MMU##p(); \
    ZXN_WRITE_MMU##p(v); \

#define POP_PAGE(p) ZXN_WRITE_MMU##p(page##p);

#endif
