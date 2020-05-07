#include <cstddef>
#include <cstdint>
#include <type_traits>

int main() {
  static_assert(!std::is_same<size_t,uint32_t>::value, "size_t == uint32_t");
  static_assert(!std::is_same<size_t,uint64_t>::value, "size_t == uint64_t");
}
