#pragma once
#include <Eigen/Dense>
#include <cmath>

class Layer {
public:
	Layer(int inputSize, int outputSize) :
		weights(outputSize, inputSize),
		biases(outputSize),
		output(outputSize),
		z(outputSize)
	{
		initializeWeights();
	}

	// Forward propagation
	Eigen::VectorXf forward(const Eigen::VectorXf& input) {
		lastInput = input;
		z = weights * input + biases;
		output = activate(z);
		return output;
	}

	// Activation function ReLU (for now)
	static Eigen::VectorXf activate(const Eigen::VectorXf& z) {
		return z.array().max(0.0f);
	}

	static Eigen::VectorXf activateDerivative(const Eigen::VectorXf& z) {
		return (z.array() > 0.0f).cast<float>();
	}

	Eigen::MatrixXf weights;
	Eigen::VectorXf biases;
	Eigen::VectorXf output;
	Eigen::VectorXf z;
	Eigen::VectorXf lastInput;
private:
	void initializeWeights() {
		float stddev = std::sqrt(2.0f / weights.rows());
		weights.setRandom();
		weights *= stddev;
		biases.setZero();
	}
};


class SoftmaxLayer {
public:

	Eigen::VectorXf forward(const Eigen::VectorXf& input) {
		lastInput = input;

		// Substract de max value so the max become 0 and the other values are negative
		Eigen::VectorXf expValues = (input.array() - input.maxCoeff()).exp();
		output = expValues / expValues.sum();
		return output;
	}

	Eigen::MatrixXf backward(const Eigen::VectorXf& outputGrad) {
		Eigen::MatrixXf jacobian = output * output.transpose();
		jacobian.diagonal() = output.array() * (1 - output.array());
		return jacobian * outputGrad;
	}

	Eigen::VectorXf lastInput;
	Eigen::VectorXf output;
};
