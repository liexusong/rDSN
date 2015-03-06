# pragma once

# include <rdsn/internal/rdsn_types.h>
# include <rdsn/internal/logging.h>

namespace rdsn { namespace utils {

extern void split_args(const char* args, __out std::vector<std::string>& sargs, char splitter = ' ');
extern void split_args(const char* args, __out std::list<std::string>& sargs, char splitter = ' ');

extern char* trim_string(char* s);

extern uint64_t get_random64();

extern uint64_t get_current_physical_time_ns();

class blob
{
public:
    blob() { _buffer = _data = 0;  _length = 0; }

    blob(std::shared_ptr<char>& buffer, int length)
        : _holder(buffer), _buffer(buffer.get()), _data(buffer.get()), _length(length)
    {}

    blob(std::shared_ptr<char>& buffer, int offset, int length)
        : _holder(buffer), _buffer(buffer.get()), _data(buffer.get() + offset), _length(length)
    {}

    blob(const char* buffer, int offset, int length)
        : _buffer(buffer), _data(buffer + offset), _length(length)
    {}

    blob(const blob& source)
        : _holder(source._holder), _buffer(source._buffer), _data(source._data), _length(source._length)
    {}

    void assign(std::shared_ptr<char>& buffer, int offset, int length)
    {
        _holder = buffer;
        _buffer = (buffer.get());
        _data = (buffer.get() + offset);
        _length = (length);
    }

    const char* data() const { return _data; }

    int   length() const { return _length; }

    std::shared_ptr<char> buffer() { return _holder; }

    blob range(int offset) const
    {
        rassert (offset <= _length, "offset cannot exceed the current length value");

        blob temp = *this;
        temp._data += offset;
        temp._length -= offset;        
        return temp;
    }

    blob range(int offset, int len) const
    {
        rassert (offset <= _length, "offset cannot exceed the current length value");

        blob temp = *this;
        temp._data += offset;
        temp._length -= offset;
        rassert (temp._length >= len, "buffer length must exceed the required length");
        temp._length = len;
        return temp;
    }

private:
    friend class binary_writer;
    std::shared_ptr<char>  _holder;
    const char*            _buffer;
    const char*            _data;    
    int                    _length; // data length
};


class binary_reader
{
public:
    binary_reader(blob& blob);

    template<typename T> void read_pod(__out T& val);
    template<typename T> void read(__out T& val) { rassert (false, "read of this type is not implemented"); }
    void read(__out int8_t& val) { read_pod(val); }
    void read(__out uint8_t& val) { read_pod(val); }
    void read(__out int16_t& val) { read_pod(val); }
    void read(__out uint16_t& val) { read_pod(val); }
    void read(__out int32_t& val) { read_pod(val); }
    void read(__out long& val) { read_pod(val); }
    void read(__out uint32_t& val) { read_pod(val); }
    void read(__out unsigned long& val) { read_pod(val); }
    void read(__out int64_t& val) { read_pod(val); }
    void read(__out uint64_t& val) { read_pod(val); }
    void read(__out bool& val) { read_pod(val); }

    void read(__out std::string& s);
    void read(char* buffer, int sz);
    void read(blob& blob);

    blob get_buffer() const { return _blob; }
    blob get_remaining_buffer() const { return _blob.range((int)(_ptr - _blob.data())); }
    bool is_eof() const { return _ptr >= _blob.data() + _size; }
    int  total_size() const { return _size; }
    int  get_remaining_size() const { return (int)(_blob.data() + _size - _ptr); }

private:
    blob        _blob;
    int         _size;
    const char* _ptr;
};


class binary_writer
{
public:
    binary_writer(int reservedBufferSize = 0);
    binary_writer(blob& buffer);
    ~binary_writer();

    uint16_t write_placeholder();
    template<typename T> void write_pod(const T& val, uint16_t pos = 0xffff);
    template<typename T> void write(const T& val, uint16_t pos = 0xffff) { rassert(false, "write of this type is not implemented"); }
    void write(const int8_t& val, uint16_t pos = 0xffff) { write_pod(val, pos); }
    void write(const uint8_t& val, uint16_t pos = 0xffff) { write_pod(val, pos); }
    void write(const int16_t& val, uint16_t pos = 0xffff) { write_pod(val, pos); }
    void write(const uint16_t& val, uint16_t pos = 0xffff) { write_pod(val, pos); }
    void write(const int32_t& val, uint16_t pos = 0xffff) { write_pod(val, pos); }
    void write(const long& val, uint16_t pos = 0xffff) { write_pod(val, pos); }
    void write(const uint32_t& val, uint16_t pos = 0xffff) { write_pod(val, pos); }
    void write(const unsigned long& val, uint16_t pos = 0xffff) { write_pod(val, pos); }
    void write(const int64_t& val, uint16_t pos = 0xffff) { write_pod(val, pos); }
    void write(const uint64_t& val, uint16_t pos = 0xffff) { write_pod(val, pos); }
    void write(const bool& val, uint16_t pos = 0xffff) { write_pod(val, pos); }

    void write(const std::string& val, uint16_t pos = 0xffff);
    void write(const char* buffer, int sz, uint16_t pos = 0xffff);
    void write(const blob& val, uint16_t pos = 0xffff);

    void get_buffers(__out std::vector<blob>& buffers) const;
    int  get_buffer_count() const { return (int)_buffers.size(); }
    blob get_buffer() const;
    blob get_first_buffer() const;

    int total_size() const { return _total_size; }

private:
    void create_buffer_and_writer(blob* pBuffer = nullptr);

private:
    std::vector<blob>  _buffers;
    std::vector<blob>  _data;
    bool               _cur_is_placeholder;            
    int                _cur_pos;
    int                _total_size;
    int                _reserved_size_per_buffer;
    static int         _reserved_size_per_buffer_static;
};

//--------------- inline implementation -------------------
template<typename T>
inline void binary_reader::read_pod(__out T& val)
{
    if (sizeof(T) <= get_remaining_size())
    {
        memcpy((void*)&val, _ptr, sizeof(T));
        _ptr += sizeof(T);
    }
    else
    {
        rassert (false, "read beyond the end of buffer");
    }
}
        
template<typename T>
inline void binary_writer::write_pod(const T& val, uint16_t pos)
{
    write((char*)&val, (int)sizeof(T), pos);
}

inline void binary_writer::get_buffers(__out std::vector<blob>& buffers) const
{
    buffers = _data;
}

inline blob binary_writer::get_first_buffer() const
{
    return _data[0];
}

inline void binary_writer::write(const std::string& val, uint16_t pos /*= 0xffff*/)
{
    int len = (int)val.length();
    write((const char*)&len, sizeof(int), pos);
    write((const char*)&val[0], len, pos);
}

inline void binary_writer::write(const blob& val, uint16_t pos /*= 0xffff*/)
{
    int len = val.length();
    write((const char*)&len, sizeof(int), pos);
    write((const char*)val.data(), len, pos);
}

}} // end namespace rdsn::utils

