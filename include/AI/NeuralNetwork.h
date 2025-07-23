#pragma once
#include <torch/torch.h>

// Définir le réseau de neurones pour l'agent de trading
struct TradingAgentNet : torch::nn::Module {
    TradingAgentNet();

    // Forward pass qui prend la heatmap et l'état de l'agent
    std::pair<torch::Tensor, torch::Tensor> forward(torch::Tensor heatmap, torch::Tensor state,
        torch::Tensor best_asks, torch::Tensor best_bids);

    // Couches du CNN
    torch::nn::Conv2d conv1{ nullptr }, conv2{ nullptr }, conv3{ nullptr }, conv4{ nullptr };
    torch::nn::BatchNorm2d bn1{ nullptr }, bn2{ nullptr }, bn3{ nullptr }, bn4{ nullptr };
    torch::nn::MaxPool2d pool{ nullptr };

    // Couches du MLP pour l'état
    torch::nn::Linear fc_state{ nullptr };

    // Conv1D pour best_asks et best_bids
    torch::nn::Conv1d conv1d_asks{ nullptr }, conv1d_bids{ nullptr };
    torch::nn::BatchNorm1d bn1d_asks{ nullptr }, bn1d_bids{ nullptr };
    torch::nn::MaxPool1d pool1d{ nullptr };

    // Couches fully connected après concaténation
    torch::nn::Linear fc1{ nullptr }, fc2{ nullptr }, fc3{ nullptr };

    // Têtes pour la politique et la valeur
    torch::nn::Linear policy_head{ nullptr }, value_head{ nullptr };
};