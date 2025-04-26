
#ifndef LTX_DISPLAY_BUFFER_H_DEFINED
#define LTX_DISPLAY_BUFFER_H_DEFINED

namespace LTX {


    /**
        A generic circular buffer class where the writer blindly keeps writing in a circular fashion, and the reader is fairly blind
        in terms of how it reads (it starts the read sensibly, but doesn't keep an eye on concurrent writes during reading).

        As the name would suggest, this buffer is intended for use in visualizations, so the bar for thread safety correctness is a bit
        lower than if it were being used for recording or processing data that is then recorded. The main question is will it crash? And
        I'm fairly sure that it won't given we only allocate heap memory once in the constructor, and the rest of the code is just indexing
        into that buffer, in a fairly trivial way.

        At construction, you need to specify both the total capactiy of the buffer, and the maximum number of elements to read at once.
        The maximum read size should be somewhat smaller than the total capacity so that the reader always has a decent head start on the writer,
        meaning it's unlikely that part-way through a read, the writer overtakes causing one or more "tears" in the read data (as shown to the user).

        To start a read call start_read(), and then in a loop call read() until it returns false.
        
        Only one reader and one write are supported (because that's enough for our use case here).
    **/
    template <typename T>
    class alignas(64) DisplayBuffer {

    public:

        DisplayBuffer(size_t capacity_, size_t max_read_size_) : 
            buffer(std::make_unique<T[]>(capacity_)), 
            capacity(capacity_),
            writer_at(0),
            writer_used(0),
            reader_at(0),
            reader_remaining(0),
            buffer_copy(buffer.get()),
            capacity_copy(capacity_),
            max_read_size(max_read_size_)
        {
            // Note the class uses alignas(64), so offsets within the class should also be offsets relative to the start of the first cache line, which is required here.
            // We are assuming that a cache line is 64 bytes..which is basically always true as far as I understand.
            static_assert(std::is_standard_layout_v<DisplayBuffer<T>>, "DisplayBuffer must be using standard layout for the cache line logic to make sense");
            static_assert(offsetof(DisplayBuffer<T>, writer_used) + sizeof(size_t) < 64, "writer bookkeeping should go on the first cache line");
            static_assert(offsetof(DisplayBuffer<T>, reader_at) >= 64, "reader bookkeeping should go on the second cache line");
            assert(capacity > 0); // "capacity must be greater than zero");
            assert(max_read_size < capacity); // "max_read_size must be less than capacity");
        };

        ~DisplayBuffer() {};

        /* Resets the buffer to empty */
        void clear() {
            reader_remaining = 0;
            writer_used = 0;
            writer_at = 0;
            reader_at = 0;
        }

        void write(T& src, bool is_valid_write) {
            buffer[writer_at] = src;
            writer_at += is_valid_write;
            if (writer_at == capacity) {
                writer_at = 0; // wrap around writer
            }
            if (writer_used < capacity) {
                writer_used += is_valid_write;
            }
        }
        
        /* Returns the read size */
        size_t start_read() {
            if (writer_used < max_read_size) {
                reader_remaining = writer_used.load();
                reader_at = 0;
            } else {
                reader_remaining = max_read_size;
                reader_at = (writer_at + capacity - max_read_size) % capacity;
            }
            LOGC("start_read: writer_used=", writer_used.load(), " writer_at=", writer_at.load(), " reader_remaining=", reader_remaining, " reader_at=", reader_at);
            return reader_remaining;
        }
        
        bool read(T& dest) {
            dest = buffer_copy[reader_at];
            reader_at++;
            if (reader_at == capacity_copy){
                reader_at = 0; // wrap around reader
            }
            return --reader_remaining > 0;
        }


        
    private:
        // We take false sharing seriously here - see the static asserts in the constructor.

        // first cache line, used by write(), and also by start_read(), and empty()
        const std::unique_ptr<T[]> buffer;
        const size_t capacity;
        std::atomic<size_t> writer_at;
        std::atomic<size_t> writer_used;

        char padding[32];  // 4 * 8 bytes above = 32, plus 32 = 64

        // second cache line, used by read(), and also by start_read() and empty()
        std::atomic<size_t> reader_at;
        std::atomic<size_t> reader_remaining;
        const T* buffer_copy;
        const size_t capacity_copy;
        const size_t max_read_size;
    };

}


#endif // LTX_DISPLAY_BUFFER_H_DEFINED