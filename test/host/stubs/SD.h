#pragma once

#include <Arduino.h>

#include <cstring>
#include <map>
#include <string>
#include <vector>

constexpr uint8_t FILE_READ = 0x01;
constexpr uint8_t FILE_WRITE = 0x02;

class File
{
  public:
    File() = default;

    File(std::vector<uint8_t>* data, const bool writable) : _data(data), _writable(writable) {}

    explicit operator bool() const { return _data != nullptr; }

    size_t size() const { return (_data != nullptr) ? _data->size() : 0U; }

    int read(uint8_t* buffer, size_t length)
    {
        if (_data == nullptr || buffer == nullptr || length == 0U)
        {
            return 0;
        }

        const size_t available = (_cursor < _data->size()) ? (_data->size() - _cursor) : 0U;
        const size_t bytesToRead = (length < available) ? length : available;
        if (bytesToRead == 0U)
        {
            return 0;
        }

        std::memcpy(buffer, _data->data() + _cursor, bytesToRead);
        _cursor += bytesToRead;
        return static_cast<int>(bytesToRead);
    }

    int write(const uint8_t* buffer, size_t length)
    {
        if (_data == nullptr || !_writable || buffer == nullptr || length == 0U)
        {
            return 0;
        }

        if (_cursor > _data->size())
        {
            _data->resize(_cursor);
        }

        const size_t requiredSize = _cursor + length;
        if (requiredSize > _data->size())
        {
            _data->resize(requiredSize);
        }

        std::memcpy(_data->data() + _cursor, buffer, length);
        _cursor += length;
        return static_cast<int>(length);
    }

    void flush() {}
    void close() { _data = nullptr; }

  private:
    std::vector<uint8_t>* _data = nullptr;
    size_t _cursor = 0U;
    bool _writable = false;
};

class SDClass
{
  public:
    bool begin(uint32_t, byte) { return true; }
    void end() {}

    bool exists(const char* path) const { return _files.find(normalizePath(path)) != _files.end(); }

    bool remove(const char* path) { return _files.erase(normalizePath(path)) > 0U; }

    File open(const char* path, uint8_t mode)
    {
        const std::string key = normalizePath(path);
        if ((mode & FILE_WRITE) != 0U)
        {
            return File(&_files[key], true);
        }

        auto it = _files.find(key);
        if (it == _files.end())
        {
            return File();
        }

        return File(&it->second, false);
    }

    void setFileContents(const char* path, const std::vector<uint8_t>& data) { _files[normalizePath(path)] = data; }

    void clear() { _files.clear(); }

  private:
    static std::string normalizePath(const char* path) { return (path != nullptr) ? std::string(path) : std::string(); }

    std::map<std::string, std::vector<uint8_t>> _files;
};

inline SDClass SD;
