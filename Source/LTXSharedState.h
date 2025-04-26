#include <atomic>

#ifndef LTXSHARED_STATE
#define LTXSHARED_STATE

/*
Using globals here is exceptionally hacky, but I couldn't find any other way using the plugin API to let the user specify some custom headers for the record
engine to write. Any of the plugins here can update these values and they will be written at the start of the recording.
*/
namespace LTX {
    namespace SharedState {
        extern std::atomic<int> window_min_x;
        extern std::atomic<int> window_max_x;
        extern std::atomic<int> window_max_y;
        extern std::atomic<int> window_min_y;
        extern std::atomic<float> pixels_per_metre;
    }
}

#endif