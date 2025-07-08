#pragma once
#include "Layer.h"
#include "engine/OrderBook.h"

// Variables to print in the ImGui console
struct AIState {
	Eigen::VectorXf lastOutputs = Eigen::VectorXf::Zero(3);
	std::string lastDecision = "HOLD";
	float lastUpdateTime = 0.0f;
};


class NeuralNetwork {
public:
	NeuralNetwork(const std::vector<int>& topology) {
		if (topology.size() < 2) {
			throw std::invalid_argument("Topology must have at least 2 different layers");
		}

		for (size_t i = 0; i < topology.size() - 1; ++i) {
			if (i < topology.size() - 2) {
				layers.emplace_back(topology[i], topology[i + 1]);
			}
			else {
				layers.emplace_back(topology[i], topology[i + 1]);
			}
		}
	}

	Eigen::VectorXf predict(const Eigen::VectorXf& input) {
		Eigen::VectorXf activation = input;

		for (auto& layer : layers) {
			activation = layer.forward(activation);
		}
		return outputLayer.forward(activation);
	}

	// Debugging purposes
	const std::vector<Layer>& getLayers() const { return layers; }
	const SoftmaxLayer& getSoftmaxLayer() const { return outputLayer; }

	AIState aiState;

private:
	std::vector<Layer> layers;
	SoftmaxLayer outputLayer;
};


Eigen::VectorXf prepareAIInputs(const OrderBook& ob,
	const BookSnapshot& snapshot,
	float& lastBestBid, float& lastBestAsk,
	float& lastBidVolume, float& lastAskVolume) {

	Eigen::VectorXf inputs(11);

	float currentBestBid = ob.getCurrentBestBid();
	float currentBestAsk = ob.getCurrentBestAsk();

	float currentBidVolume = 0.0f;
	if (snapshot.prices.count(currentBestBid)) {
		for (const auto& order : snapshot.prices.at(currentBestBid)) {
			currentBidVolume += order.size;
		}
	}

	float currentAskVolume = 0.0f;
	if (snapshot.prices.count(currentBestAsk)) {
		for (const auto& order : snapshot.prices.at(currentBestAsk)) {
			currentAskVolume += order.size;
		}
	}

	float bidChange = currentBestBid - lastBestBid;
	float askChange = currentBestAsk - lastBestAsk;
	float spreadChange = (currentBestAsk - currentBestBid) - (lastBestAsk - lastBestBid);
	float volumeImbalance = currentAskVolume - currentBidVolume;
	float bestBidVolumeChange = currentBidVolume - lastBidVolume;
	float bestAskVolumeChange = currentAskVolume - lastAskVolume;

	inputs <<
		(currentBestAsk - initialPrice) / depth,
		(currentBestBid - initialPrice) / depth,
		currentAskVolume,
		currentBidVolume,
		(currentBestAsk - currentBestBid),
		askChange,
		bidChange,
		spreadChange,
		volumeImbalance,
		bestAskVolumeChange,
		bestBidVolumeChange;

	lastBestAsk = currentBestAsk;
	lastBestBid = currentBestBid;
	lastAskVolume = currentAskVolume;
	lastBidVolume = currentBidVolume;

	return inputs;
};