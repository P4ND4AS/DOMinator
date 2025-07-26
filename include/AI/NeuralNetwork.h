#pragma once
#include <torch/torch.h>

class TradingAgentNet : public torch::nn::Module {
public:
    TradingAgentNet();

    std::pair<torch::Tensor, torch::Tensor> forward(torch::Tensor state, torch::Tensor best_asks, torch::Tensor best_bids);
        
private:
    torch::nn::Linear fc_state{ nullptr };
    torch::nn::Conv1d conv1d_asks_1{ nullptr }, conv1d_asks_2{ nullptr };
    torch::nn::BatchNorm1d bn1d_asks_1{ nullptr }, bn1d_asks_2{ nullptr };
    torch::nn::Conv1d conv1d_bids_1{ nullptr }, conv1d_bids_2{ nullptr };
    torch::nn::BatchNorm1d bn1d_bids_1{ nullptr }, bn1d_bids_2{ nullptr };
    torch::nn::MaxPool1d pool1d{ nullptr };
    torch::nn::Linear fc1{ nullptr }, fc2{ nullptr }, fc3{ nullptr };
    torch::nn::Linear policy_head{ nullptr }, value_head{ nullptr };
};