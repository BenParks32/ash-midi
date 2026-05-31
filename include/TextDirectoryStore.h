#pragma once

#include <Arduino.h>

struct TextFilePathEntry
{
    static constexpr size_t MaxPathLength = 96;

    char path[MaxPathLength];
};

class ITextDirectoryStore
{
public:
    virtual ~ITextDirectoryStore() = default;

    virtual size_t listTextFiles(const char *directoryPath,
                                 TextFilePathEntry *entries,
                                 size_t maxEntries) const = 0;
};
