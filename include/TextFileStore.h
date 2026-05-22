#pragma once

#include <Arduino.h>
#include <stddef.h>

class ITextFileStore
{
  public:
    virtual ~ITextFileStore() = default;

    virtual bool readTextFile(const char* path, char* buffer, size_t bufferSize) const = 0;
};
