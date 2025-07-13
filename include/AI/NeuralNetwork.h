#pragma once
#include <Eigen/Dense>
#include <fstream>
#include "engine/OrderBook.h"
#include <nlohmann/json.hpp>
#include <vector>



struct ConvConfig {
	int inChannels;
	int outChannels;
	int kernelSize;
	int stride;
	int padding;
};

struct AIConfig {
	int inputHeight;
	int inputWidth;
	int agentInputSize;
	int agentHiddenSize;
	std::vector<ConvConfig> convLayers;
	int denseLayerSize;
	int numActions;
};

AIConfig loadConfig(const std::string& filename);



enum ActivationType { RELU, NONE };

class ConvLayer {
public:
	ConvLayer(int inputDepth, int numFilters, int filterSize, int stride = 1,
		int padding = 0, ActivationType activation = RELU);

	std::vector<Eigen::MatrixXf> forward(const std::vector<Eigen::MatrixXf>& input);


private:
	int inputDepth;
	int numFilters;
	int filterSize;
	int stride;
	int padding;
	ActivationType activation;

	std::vector<std::vector<Eigen::MatrixXf>> filters;
	std::vector<float> biases;

	Eigen::MatrixXf convolve2D(const Eigen::MatrixXf& input, const Eigen::MatrixXf& kernel, int stride, int padding);
	Eigen::MatrixXf applyActivation(const Eigen::MatrixXf& input);
};


//------------ CONVOLUTIONAL NEURAL NETWORK ---------------

class CNN {
public:
	CNN(const AIConfig& config);

	std::vector<Eigen::MatrixXf> forward(const std::vector<Eigen::MatrixXf>& input);
	Eigen::VectorXf flatten(const std::vector<Eigen::MatrixXf>& featureMaps);
	int getFlattenedSize() const;

private:
	std::vector<ConvLayer> layers;
	int finalHeight;
	int finalWidth;
	int finalChannels;
};



// --------------- POLICY VALUE NETWORK ---------------

class PolicyValueNet {
public:
	PolicyValueNet(const AIConfig& config);

	std::pair<Eigen::VectorXf, float> forward(const Eigen::MatrixXf& input_image,
		const Eigen::VectorXf& agent_state);

private:
	Eigen::MatrixXf agent_fc_weights;
	Eigen::VectorXf agent_fc_bias;

	CNN cnn;

	Eigen::MatrixXf fc1_weights;
	Eigen::VectorXf fc1_bias;

	Eigen::MatrixXf policy_head_weights;
	Eigen::VectorXf policy_head_bias;

	Eigen::MatrixXf value_head_weights;
	Eigen::VectorXf value_head_bias;

	Eigen::VectorXf relu(const Eigen::VectorXf& x);
	Eigen::VectorXf softmax(const Eigen::VectorXf& x);
};
