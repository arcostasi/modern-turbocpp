#include <algorithm>
#include <format>
#include <iostream>
#include <ranges>
#include <vector>

template <typename T>
concept Printable = requires(T a) { std::cout << a; };

void printValue(Printable auto val) {
  std::cout << std::format("Square: {}\n", val);
}

int main() {
  std::cout << "========================================\n";
  std::cout << " Hello from Modern Turbo C++!\n";
  std::cout << " Compiled with LLVM Clang in C++23\n";
  std::cout << "========================================\n\n";

  std::vector<int> nums = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

  // Modern C++20/23 ranges pipeline
  auto even_squares = nums |
                      std::views::filter([](int n) { return n % 2 == 0; }) |
                      std::views::transform([](int n) { return n * n; });

  std::cout << "Even squares computation:\n";
  for (int sq : even_squares) {
    printValue(sq);
  }

  std::cout << "\nModern C++23 execution complete.\n";
  return 0;
}
