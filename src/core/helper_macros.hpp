#pragma once

#define SINGLETON_IMPL(CLASS) \
public: \
CLASS(const CLASS&) = delete; \
CLASS& operator=(const CLASS&) = delete; \
 \
static CLASS& instance() { \
    static CLASS inst; \
    return inst; \
}

#define DEFINE_GET_SET(FIELD) \
auto& FIELD() const { return _##FIELD; } \
void FIELD(const decltype(_##FIELD)& value) { _##FIELD = value; }
