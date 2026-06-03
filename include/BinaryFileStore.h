#pragma once

#include <Arduino.h>
#include <stddef.h>

class IBinaryFileStore
{
  public:
    virtual ~IBinaryFileStore() = default;

    virtual bool readBinaryFile(const char* path, uint8_t*& buffer, size_t& size) const = 0;
};
