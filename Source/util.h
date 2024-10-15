
#if defined(__GNUC__) || defined(__clang__)
#define BSWAP16(x) __builtin_bswap16(x)
#define BSWAP32(x) __builtin_bswap32(x)
#elif defined(_MSC_VER)
#include <cstdlib>  // For _byteswap_ushort and _byteswap_ulong
#define BSWAP16(x) _byteswap_ushort(x)
#define BSWAP32(x) _byteswap_ulong(x)
#else
#define BSWAP16(x) (((x & 0x00FF) << 8) | \
                        ((x & 0xFF00) >> 8))

#define BSWAP32(x) (((x & 0x000000FF) << 24) | \
                        ((x & 0x0000FF00) << 8)  | \
                        ((x & 0x00FF0000) >> 8)  | \
                        ((x & 0xFF000000) >> 24))
#endif


