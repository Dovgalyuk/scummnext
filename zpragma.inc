// code start address

#pragma output CRT_ORG_CODE = 0x6a00

// stack location

#pragma output REGISTER_SP  = 0x0

// indicate we are not returning to basic, reduces crt size

#pragma output CRT_ON_EXIT  = 0x10001

// control size of printf

//#pragma printf = "%u"

// heap sizes

#pragma output CLIB_STDIO_HEAP_SIZE  = 0
