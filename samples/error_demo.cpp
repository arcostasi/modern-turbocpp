#include <iostream>

// Sample program demonstrating Modern Turbo C++ error and warning detection with LLVM
int computeSum(int a, int b) {
    int unusedVariable = 123;   // Trigger warning: unused variable
    int result = a + b          // Trigger error: missing semicolon
    return result;
}

int main() {
    std::cout << "Sum: " << computeSum(10, 20) << std::endl;
    return 0;
}
