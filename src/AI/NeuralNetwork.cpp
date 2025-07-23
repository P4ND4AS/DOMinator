#include "AI/NeuralNetwork.h"


TradingAgentNet::TradingAgentNet() {
    // CNN pour la heatmap (401x800x1)
    conv1 = register_module("conv1", torch::nn::Conv2d(
        torch::nn::Conv2dOptions(1, 32, 3).stride(2)));
    bn1 = register_module("bn1", torch::nn::BatchNorm2d(32));
    conv2 = register_module("conv2", torch::nn::Conv2d(
        torch::nn::Conv2dOptions(32, 64, 3).stride(1)));
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

    // Convolutions 1D pour best_asks et best_bids
    conv1d_asks = register_module("conv1d_asks", torch::nn::Conv1d(
        torch::nn::Conv1dOptions(1, 16, 3).stride(1).padding(1))); // [T_max, 1, 50] -> [T_max, 16, 50]
    bn1d_asks = register_module("bn1d_asks", torch::nn::BatchNorm1d(16));
    conv1d_bids = register_module("conv1d_bids", torch::nn::Conv1d(
        torch::nn::Conv1dOptions(1, 16, 3).stride(1).padding(1))); // [T_max, 1, 50] -> [T_max, 16, 50]
    bn1d_bids = register_module("bn1d_bids", torch::nn::BatchNorm1d(16));
    pool1d = torch::nn::MaxPool1d(torch::nn::MaxPool1dOptions(2).stride(2)); // Réduire à [T_max, 16, 25]

    // Couches fully connected après concaténation
    fc1 = register_module("fc1", torch::nn::Linear(63536, 1024));
    fc2 = register_module("fc2", torch::nn::Linear(1024, 512));
    fc3 = register_module("fc3", torch::nn::Linear(512, 128));

    // Têtes pour la politique et la valeur
    policy_head = register_module("policy_head", torch::nn::Linear(128, 3));
    value_head = register_module("value_head", torch::nn::Linear(128, 1));

    to(torch::kCUDA);
}


std::pair<torch::Tensor, torch::Tensor> TradingAgentNet::forward(torch::Tensor heatmap, torch::Tensor state, 
    torch::Tensor best_asks, torch::Tensor best_bids) {

    auto x = torch::relu(bn1->forward(conv1->forward(heatmap)));
    x = pool->forward(x);
    x = torch::relu(bn2->forward(conv2->forward(x)));
    x = pool->forward(x);
    x = torch::relu(bn3->forward(conv3->forward(x)));
    x = torch::relu(bn4->forward(conv4->forward(x)));
    x = x.view({ x.size(0), -1 }); 
    
    // MLP pour l'état
    auto s = torch::relu(fc_state->forward(state));

    auto asks = torch::relu(bn1d_asks->forward(conv1d_asks->forward(best_asks)));
    asks = pool1d->forward(asks);
    asks = asks.view({ asks.size(0), -1 });

    auto bids = torch::relu(bn1d_bids->forward(conv1d_bids->forward(best_bids)));
    bids = pool1d->forward(bids);
    bids = bids.view({ bids.size(0), -1 });
    
    // Concaténation
    auto combined = torch::cat({ x, s, asks, bids }, 1);


    // Couches fully connected
    combined = torch::relu(fc1->forward(combined));
    combined = torch::relu(fc2->forward(combined));
    combined = torch::relu(fc3->forward(combined));

    // Sorties
    auto policy = torch::softmax(policy_head->forward(combined), 1); // Probabilités pour BUY, SELL, WAIT
    auto value = value_head->forward(combined); // Valeur de l'état

    return { policy, value };
}