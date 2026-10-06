// USe Segger RTT View printouts only in debug mode

#ifdef DEBUG
	#include "SEGGER_RTT.h"
#endif

#ifdef ENABLE_RTT_LOGGING
    // Wrapper that passes all arguments directly to the printf function
    #define RTT_printf(buffer, format, ...) SEGGER_RTT_printf(buffer, format, ##__VA_ARGS__)
	#define RTT_puts(buffer, string) SEGGER_RTT_WriteString(buffer, string)

#else
    // Wrapper that compiles down to absolutely nothing, saving CPU cycles and memory
    #define RTT_printf(buffer, format, ...) ((void)0)
	#define RTT_puts(buffer, string) ((void)0)
#endif
