// Shim minimale di Arduino.h per compilare la logica pura in nativo
// (test su host: nessun hardware coinvolto).
#pragma once
#include <cstdint>
#include <cstring>
#include <cctype>
#include <cstdio>
#include <string>
#include <algorithm>

using std::min;
using std::max;

class String {
public:
    String() = default;
    String(const char* s) : _s(s) {}
    String(const std::string& s) : _s(s) {}
    size_t length() const { return _s.size(); }
    char charAt(size_t i) const { return _s[i]; }
    void remove(size_t idx) { _s.erase(idx); }
    void reserve(size_t) {}
    String& operator+=(char c) { _s += c; return *this; }
    String& operator+=(const String& o) { _s += o._s; return *this; }
    String& operator=(const char* s) { _s = s; return *this; }
    bool operator==(const char* s) const { return _s == s; }
    bool operator==(const String& o) const { return _s == o._s; }
    String substring(unsigned a, unsigned b) const { return String(_s.substr(a, b - a)); }
    const char* c_str() const { return _s.c_str(); }
    friend String operator+(const String& a, const String& b) { return String(a._s + b._s); }
private:
    std::string _s;
};
