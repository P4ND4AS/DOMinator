#include "AI/NeuralNetwork.h"


TradingAgentNet::TradingAgentNet() {
    // CNN pour la heatmap (401x800x1)
    conv1 = register_module("conv1", torch::nn::Conv2d(
        torch::nn::Conv2dOptions(1, 32, 8).stride(4)));
    bn1 = register_module("bn1", torch::nn::BatchNorm2d(32));
    conv2 = register_module("conv2", torch::nn::Conv2d(
        torch::nn::Conv2dOptions(32, 64, 5).stride(2)));
    bn2 = register_module("bn2", torch::nn::BatchNorm2d(64));
    conv3 = register_module("conv3", torch::nn::Conv2d(
        torch::nn::Conv2dOptions(64, 128, 3).stride(1).padding(1)));
    bn3 = register_module("bn3", torch::nn::BatchNorm2d(128));
    conv4 = register_module("conv4", torch::nn::Conv2d(
        torch::nn::Conv2dOptions(128, 256, 3).stride(1).padding(1)));
    bn4 = register_module("bn4", torch::nn::BatchNorm2d(256));
    pool = torch::nn::MaxPool2d(torch::nn::MaxPool2dOptions(2).stride(2));

    // MLP pour l'état de l'agent
    fc_state = register_module("fc_state", torch::nn::Linear(1, 16));

    // Couches fully connected après concaténation
    fc1 = register_module("fc1", torch::nn::Linear(256 * 11 * 24 + 16, 1024));
    fc2 = register_module("fc2", torch::nn::Linear(1024, 512));
    fc3 = register_module("fc3", torch::nn::Linear(512, 128));

    // Têtes pour la politique et la valeur
    policy_head = register_module("policy_head", torch::nn::Linear(128, 3));
    value_head = register_module("value_head", torch::nn::Linear(128, 1));

    to(torch::kCUDA);
}


std::pair<torch::Tensor, torch::Tensor> TradingAgentNet::forward(torch::Tensor heatmap, torch::Tensor state) {
    // CNN pour la heatmap
    auto x = torch::relu(bn1->forward(conv1->forward(heatmap)));
    x = pool->forward(x);
    x = torch::relu(bn2->forward(conv2->forward(x)));
    x = pool->forward(x);
    x = torch::relu(bn3->forward(conv3->forward(x)));
    x = torch::relu(bn4->forward(conv4->forward(x)));
    x = x.view({ x.size(0), -1 }); // Aplatir : 256 * 7 * 20

    // MLP pour l'état
    auto s = torch::relu(fc_state->forward(state));
    // Concaténation
    auto combined = torch::cat({ x, s }, 1);

    // Couches fully connected
    combined = torch::relu(fc1->forward(combined));
    combined = torch::relu(fc2->forward(combined));
    combined = torch::relu(fc3->forward(combined));

    // Sorties
    auto policy = torch::softmax(policy_head->forward(combined), 1); // Probabilités pour BUY, SELL, WAIT
    auto value = value_head->forward(combined); // Valeur de l'état

    return { policy, value };
}