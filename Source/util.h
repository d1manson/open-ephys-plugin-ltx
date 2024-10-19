
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


/*
    Converts each element in the src float array to an int8 array by dividing by two and clamping to the int8 range.
*/
template <size_t N, int Min, int Max>
inline void float32sToInt8s(const float* src, int8* dest) {
    static_assert(N == 40, "expected 40 samples per spike");
    static_assert(Min == -250 && Max == 250, "expected input range [-250,250]");

    // TODO(Optimisation): can we do better here using SIMD?
    //       For N=40, use SIMD 512bit intrinsics three times, rather than a loop
    //       may need to copy into a properly aligned float[48] buffer, with zero padding at the end
    //       should be able to make that static as the zeros will never be overwritten
    //       can then use 16,16,16 float operations. Think there might even be a builtin way to "saturate" the cast
    for (int i = 0; i < N; i++) {
        int32 v = static_cast<int32>(src[i]) / 2; // see static_assert above regarding expected input range [-250,250]
        dest[i] = std::min(std::max(v, -128), 127);
    }
}


/*
    Starting with the zeroth element in the src float array, it takes steps through the src float array, dividing by two
    and clamping to the int8 range, storing the result in the smaller out array. Returns the number of downsampled elements.
*/
template <size_t OutSizeMax, int Min, int Max, int Step>
inline int64 float32sToInt8sDownsampled(const float* src, int8* dest, int size) {

    // TODO(Optimisation): can we do better here using SIMD?
    static_assert(Min == -250 && Max == 250, "expected input range [-250,250]");
    int64 out = 0;
    for (int in = 0; in < size && out < OutSizeMax; in += Step, out++) {
        int32 v = static_cast<int32>(src[in]) / 2; // see static_assert above regarding expected input range [-250,250]
        dest[out] = std::min(std::max(v, -128), 127);
    }
    return out;
}

/*
    Multiplies each float in the array by the given factor, making the change in place.
*/
inline void multiply(uint32 size, float* buffer, float factor) {
    // TODO(Optimisation): can we do better here using SIMD?
    for (; size; size--) {
        buffer[0] *= factor;
        buffer++;
    }
}