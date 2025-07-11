#pragma once
#include <Eigen/Dense>
#include "engine/OrderBook.h"

class Layer {
public:
	Eigen::MatrixXf weights;
	Eigen::VectorXf biases;
	Eigen::VectorXf output;

	Layer(int inputSize, int outputSize);

	Eigen::VectorXf forward(const Eigen::VectorXf& input);

};