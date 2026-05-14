#pragma once

#include <Arduino.h>
#include <stddef.h>

class ISmallFileStore
{
  public:
    virtual ~ISmallFileStore() = default;

    virtual bool readSmallFile(const char* path, uint8_t* buffer, size_t expectedSize) const = 0;
    virtual bool writeSmallFile(const char* path, const uint8_t* data, size_t size) = 0;
};
