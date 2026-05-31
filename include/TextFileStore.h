#pragma once

#include <Arduino.h>
#include <stddef.h>

constexpr size_t MaxTextFilePathLength = 127;

class ITextFileStore
{
  public:
    virtual ~ITextFileStore() = default;

    virtual bool readTextFile(const char* path, char* buffer, size_t bufferSize) const = 0;
};
