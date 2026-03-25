#pragma once
#include "Gfx.h"
#include <Arduino.h>
#include <SD.h>

const uint SZ_ICON_FOOTSWITCH = 48;

class Resource
{
  public:
    Resource(const uint16_t size, const uint16_t* data) : _size(size), _data(data) {}
    ~Resource() { delete[] _data; }

  private:
    Resource() = delete;
    Resource(const Resource&) = delete;
    Resource& operator=(const Resource&) = delete;

  public:
    const uint16_t size() const { return _size; }
    const uint16_t* data() const { return _data; }

  private:
    const uint16_t _size;
    const uint16_t* _data;
};

class Icon : public Resource
{
  public:
    Icon(Size size, const uint16_t* data) : Resource(size.width * size.height, data), _iconSize(size) {}

  private:
    Icon() = delete;
    Icon(const Icon&) = delete;
    Icon& operator=(const Icon&) = delete;

  public:
    const Size& iconSize() const { return _iconSize; }

  private:
    const Size _iconSize;
};

class Resources
{
  public:
    Resources(const byte sdPin);
    ~Resources();

  private:
    Resources() = delete;
    Resources(const Resources&) = delete;
    Resources& operator=(const Resources&) = delete;

  private:
    const uint16_t* readFile(const char* path, const uint16_t expectedSize) const;
    const Icon* const loadIcon(const char* path, const Size& size) const;

  private:
    const byte _sdPin;
    const Icon* FootSwitchIcon;

  public:
    bool init();
    bool loadAll();
    const Icon& footSwitchIcon() const { return *FootSwitchIcon; }
};