#pragma once
#include <torch/torch.h>

// D�finir le r�seau de neurones pour l'agent de trading
struct TradingAgentNet : torch::nn::Module {
    TradingAgentNet();

    // Forward pass qui prend la heatmap et l'�tat de l'agent
    std::pair<torch::Tensor, torch::Tensor> forward(torch::Tensor heatmap, torch::Tensor state);

    // Couches du CNN
    torch::nn::Conv2d conv1{ nullptr }, conv2{ nullptr }, conv3{ nullptr }, conv4{ nullptr };
    torch::nn::BatchNorm2d bn1{ nullptr }, bn2{ nullptr }, bn3{ nullptr }, bn4{ nullptr };
    torch::nn::MaxPool2d pool{ nullptr };

    // Couches du MLP pour l'�tat
    torch::nn::Linear fc_state{ nullptr };

    // Couches fully connected apr�s concat�nation
    torch::nn::Linear fc1{ nullptr }, fc2{ nullptr }, fc3{ nullptr };

    // T�tes pour la politique et la valeur
    torch::nn::Linear policy_head{ nullptr }, value_head{ nullptr };
};