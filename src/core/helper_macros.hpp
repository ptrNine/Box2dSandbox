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

#define DECLARE_GET_SET(FIELD) \
auto& FIELD() const { return _##FIELD; } \
void FIELD(const decltype(_##FIELD)& value) { _##FIELD = value; }


#define DECLARE_SELF_FABRICS(CLASS)  \
using SharedPtr = std::shared_ptr<CLASS>; \
using UniquePtr = std::unique_ptr<CLASS>; \
template<typename... Args>          \
static auto createUnique(Args&&... args) { \
    return std::make_unique<CLASS>(args...); \
} \
template<typename... Args>          \
static auto createShared(Args&&... args) { \
    return std::make_shared<CLASS>(args...); \
}