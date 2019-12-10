#include <string>
#include <functional>

#include "IndexedHashStorage.hpp"

template <typename FuncT>
using FunctionMap = IndexedHashStorage<std::string, std::function<FuncT>>;
