#include "AI/NeuralNetwork.h"
#include <random>
#include <cmath>
#include <iostream>
using json = nlohmann::json;


AIConfig loadConfig(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Erreur : impossible d’ouvrir le fichier de configuration : " << filename << "\n";
        std::exit(EXIT_FAILURE);
    }

    json j;
    try {
        file >> j;
    }
    catch (const std::exception& e) {
        std::cerr << "Erreur lors du parsing JSON : " << e.what() << "\n";
        std::exit(EXIT_FAILURE);
    }

    AIConfig config;
    config.inputHeight = j["input_height"];
    config.inputWidth = j["input_width"];
    config.denseLayerSize = j["dense_layer_size"];
    config.numActions = j["num_actions"];

    for (const auto& layer : j["cnn_layers"]) {
        config.convLayers.push_back({
            layer["in"],
            layer["out"],
            layer["kernel"],
            layer["stride"],
            layer["padding"]
            });
    }
    return config;
}




ConvLayer::ConvLayer(int inputDepth, int numFilters, int filterSize, int stride, int padding,
	ActivationType activation)
	: inputDepth(inputDepth), numFilters(numFilters), filterSize(filterSize),
	stride(stride), padding(padding), activation(activation)
{
	std::default_random_engine generator;
	std::normal_distribution<float> distribution(0.0, 1.0);

	filters.resize(numFilters);
	biases.resize(numFilters, 0.0f);

	for (int f = 0; f < numFilters; ++f) {
		filters[f].resize(inputDepth);
		for (int c = 0; c < inputDepth; ++c) {
			filters[f][c] = Eigen::MatrixXf(filterSize, filterSize);
			for (int i = 0; i < filterSize; ++i)
				for (int j = 0; j < filterSize; ++j)
					filters[f][c](i, j) = distribution(generator) * 0.1f;
		}
	}
}



std::vector<Eigen::MatrixXf> ConvLayer::forward(const std::vector<Eigen::MatrixXf>& input) {
    std::vector<Eigen::MatrixXf> output(numFilters);

    for (int f = 0; f < numFilters; ++f) {
        Eigen::MatrixXf summed;

        for (int c = 0; c < inputDepth; ++c) {
            Eigen::MatrixXf conv = convolve2D(input[c], filters[f][c], stride, padding);
            if (c == 0)
                summed = conv;
            else
                summed += conv;
        }

        summed.array() += biases[f];
        output[f] = applyActivation(summed);
    }

    return output;
}


Eigen::MatrixXf ConvLayer::convolve2D(const Eigen::MatrixXf& input, const Eigen::MatrixXf& kernel, int stride, int padding) {
    int inputSize = input.rows();
    int kernelSize = kernel.rows();


    Eigen::MatrixXf padded = Eigen::MatrixXf::Zero(inputSize + 2 * padding, inputSize + 2 * padding);
    padded.block(padding, padding, inputSize, inputSize) = input;

    int outputSize = (padded.rows() - kernelSize) / stride + 1;
    Eigen::MatrixXf output(outputSize, outputSize);

    for (int i = 0; i < outputSize; ++i) {
        for (int j = 0; j < outputSize; ++j) {
            output(i, j) = (padded.block(i * stride, j * stride, kernelSize, kernelSize).cwiseProduct(kernel)).sum();
        }
    }

    return output;
}


Eigen::MatrixXf ConvLayer::applyActivation(const Eigen::MatrixXf& input) {
    if (activation == RELU)
        return input.cwiseMax(0.0f);
    else
        return input;
}


//------------ CONVOLUTIONAL NEURAL NETWORK ---------------

CNN::CNN(const AIConfig& config)
    : finalHeight(config.inputHeight), finalWidth(config.inputWidth)
{
    for (const auto& layer : config.convLayers) {
        layers.emplace_back(layer.inChannels, layer.outChannels, layer.kernelSize, layer.stride, layer.padding);

        // Calcul des dimensions après chaque convolution
        finalHeight = (finalHeight + 2 * layer.padding - layer.kernelSize) / layer.stride + 1;
        finalWidth = (finalWidth + 2 * layer.padding - layer.kernelSize) / layer.stride + 1;
    }
    finalChannels = config.convLayers.back().outChannels;
}



std::vector<Eigen::MatrixXf> CNN::forward(const std::vector<Eigen::MatrixXf>& input) {
    std::vector<Eigen::MatrixXf> current = input;
    for (auto& layer : layers) {
        current = layer.forward(current);
    }
    return current;
}

Eigen::VectorXf CNN::flatten(const std::vector<Eigen::MatrixXf>& featureMaps) {
    Eigen::VectorXf flat(finalChannels * finalHeight * finalWidth);
    int index = 0;
    for (const auto& fm : featureMaps) {
        for (int i = 0; i < fm.rows(); ++i) {
            for (int j = 0; j < fm.cols(); ++j) {
                flat(index++) = fm(i, j);
            }
        }
    }
    return flat;
}


int CNN::getFlattenedSize() const {
    return finalChannels * finalHeight * finalWidth;
}



// --------------- POLICY VALUE NETWORK ---------------


PolicyValueNet::PolicyValueNet(const AIConfig& config)
    : cnn(config)  // Le CNN prend maintenant tout le config
{

    int flattenedSize = cnn.getFlattenedSize();
    if (flattenedSize <= 0) {
        std::cerr << "ERREUR: taille aplatie CNN invalide: " << flattenedSize << "\n";
        std::exit(1);
    }

    int hiddenSize = config.denseLayerSize;
    int numActions = config.numActions;

    // Initialisation des poids et biais pour la couche fully connected
    fc1_weights = Eigen::MatrixXf::Random(hiddenSize, flattenedSize);
    fc1_bias = Eigen::VectorXf::Random(hiddenSize);

    // Tête policy : génère une probabilité pour chaque action
    policy_head_weights = Eigen::MatrixXf::Random(numActions, hiddenSize);
    policy_head_bias = Eigen::VectorXf::Random(numActions);

    // Tête value : produit un scalaire représentant V(s_t)
    value_head_weights = Eigen::MatrixXf::Random(1, hiddenSize);
    value_head_bias = Eigen::VectorXf::Random(1);

}



std::pair<Eigen::VectorXf, float> PolicyValueNet::forward(const Eigen::MatrixXf& input) {
    std::vector<Eigen::MatrixXf> inputChannels = { input }; // input canaux = 1

    auto convOutput = cnn.forward(inputChannels);            
    Eigen::VectorXf flat = cnn.flatten(convOutput);           
    Eigen::VectorXf hidden = relu(fc1_weights * flat + fc1_bias); 

    Eigen::VectorXf logits = policy_head_weights * hidden + policy_head_bias;
    Eigen::VectorXf policy = softmax(logits);

    float value = (value_head_weights * hidden + value_head_bias)(0);

    return { policy, value };
}



Eigen::VectorXf PolicyValueNet::relu(const Eigen::VectorXf& x) {
    return x.unaryExpr([](float val) { return std::max(0.0f, val); });
}

Eigen::VectorXf PolicyValueNet::softmax(const Eigen::VectorXf& x) {
    Eigen::VectorXf exp_x = x.unaryExpr([](float val) { return std::exp(val); });
    float sum = exp_x.sum();
    return exp_x / sum;
}
