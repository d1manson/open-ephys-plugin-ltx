#ifndef LTXSHARED_STATE
#define LTXSHARED_STATE

/*
This is exceptionally hacky, but I couldn't find any other way to let the user specify some custom headers for the record engine to write. 
Any of the plugins here can update these values and they will be written at the start of the recording.

These should probably be atomic for added safety, because who knows how they are being written and read. For now i'm assuming it's ok as is
given the data types are primitive.
*/
namespace LTX {
    namespace SharedState {
        extern int window_max_x;
        extern int window_max_y;
        extern double pixels_per_metre;
    }
}

#endif