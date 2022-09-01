#pragma once

#include <cstdio>
#include <string>
#include <string.h>

namespace qtr {

    // TODO: test this class
    template<size_t buf_size>
    class BufferedReader {
    private:
        FILE *_file;
        char _buf[buf_size];
        size_t _current_buf_size;
        size_t _buf_ptr;
    public:
        BufferedReader(const char *fileName) : _current_buf_size(0), _buf(0) {
            _file = std::fopen(fileName, "rb");
        }

        bool isEof() {
            if (_buf_ptr == _current_buf_size) {
                _buf_ptr = 0;
                _current_buf_size = fread(_buf, 1, buf_size, _file);
            }
            return buf_size == 0;
        }

        int get() {
            return isEof() ? EOF : _buf[_buf_ptr++];
        }

        BufferedReader &read(char *s, size_t count) {
            while (!isEof() && count != 0) {
                if (count <= _current_buf_size - _buf_ptr) {
                    memcpy(s, _buf + _buf_ptr, count);
                    _buf_ptr += count;
                    s += count;
                    count = 0;
                } else {
                    memcpy(s, _buf + _buf_ptr, _current_buf_size - _buf_ptr);
                    count -= _current_buf_size - _buf_ptr;
                    s += _current_buf_size - _buf_ptr;
                    _buf_ptr = _current_buf_size;
                }
            }
            return *this;
        }

        operator bool() const {
            return isEof();
        }

        ~BufferedReader() {
            fclose(_file);
        }
    };
}