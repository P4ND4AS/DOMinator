#include "AI/NeuralNetwork.h"
#include <random>

Layer::Layer(int inputSize, int outputSize) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<float> dist(0.0f, 1.0f / sqrt(inputSize));

    weights = Eigen::MatrixXf(outputSize, inputSize);
    biases = Eigen::VectorXf::Zero(outputSize);

    for (int i = 0; i < outputSize; ++i) {
        for (int j = 0; j < inputSize; ++j) {
            weights(i, j) = dist(gen);
        }
    }
}

Eigen::VectorXf Layer::forward(const Eigen::VectorXf& input) {
    output = (weights * input + biases).unaryExpr([](float x) {
        return std::max(0.0f, x);
        });
    return output;
}

