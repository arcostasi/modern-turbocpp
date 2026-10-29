#include <stdio.h>
#include <math.h>

// Simple single-neuron perceptron demo in C23
double sigmoid(double x) {
    return 1.0 / (1.0 + exp(-x));
}

int main(void) {
    printf("=======================================================\n");
    printf(" Modern Turbo C++ (C23 Mode) - Neural Demo\n");
    printf(" Compiled natively with LLVM Clang\n");
    printf("=======================================================\n\n");

    double weights[3] = {0.2, 0.4, -0.5};
    double inputs[3] = {1.0, 0.5, 2.0};
    double bias = -0.1;

    double sum = bias;
    for (int i = 0; i < 3; ++i) {
        sum += weights[i] * inputs[i];
    }

    double output = sigmoid(sum);
    printf("Inputs:  [%.1f, %.1f, %.1f]\n", inputs[0], inputs[1], inputs[2]);
    printf("Weights: [%.1f, %.1f, %.1f], Bias: %.1f\n", weights[0], weights[1], weights[2], bias);
    printf("Weighted sum: %f\n", sum);
    printf("Neuron Output (Sigmoid activation): %f\n\n", output);
    printf("C23 compilation and execution successful!\n");

    return 0;
}
