#include "AI/NeuralNetwork.h"


TradingAgentNet::TradingAgentNet() {
    try {
        // MLP pour l'état de l'agent
        fc_state = register_module("fc_state", torch::nn::Linear(1, 16));

        // Convolutions 1D pour best_asks
        conv1d_asks_1 = register_module("conv1d_asks_1", torch::nn::Conv1d(
            torch::nn::Conv1dOptions(1, 16, 3).stride(1).padding(1))); // [batch_size, 1, 50] -> [batch_size, 16, 50]
        bn1d_asks_1 = register_module("bn1d_asks_1", torch::nn::BatchNorm1d(16));
        conv1d_asks_2 = register_module("conv1d_asks_2", torch::nn::Conv1d(
            torch::nn::Conv1dOptions(16, 32, 3).stride(1).padding(1))); // [batch_size, 16, 50] -> [batch_size, 32, 50]
        bn1d_asks_2 = register_module("bn1d_asks_2", torch::nn::BatchNorm1d(32));

        // Convolutions 1D pour best_bids
        conv1d_bids_1 = register_module("conv1d_bids_1", torch::nn::Conv1d(
            torch::nn::Conv1dOptions(1, 16, 3).stride(1).padding(1))); // [batch_size, 1, 50] -> [batch_size, 16, 50]
        bn1d_bids_1 = register_module("bn1d_bids_1", torch::nn::BatchNorm1d(16));
        conv1d_bids_2 = register_module("conv1d_bids_2", torch::nn::Conv1d(
            torch::nn::Conv1dOptions(16, 32, 3).stride(1).padding(1))); // [batch_size, 16, 50] -> [batch_size, 32, 50]
        bn1d_bids_2 = register_module("bn1d_bids_2", torch::nn::BatchNorm1d(32));

        // Pooling
        pool1d = torch::nn::MaxPool1d(torch::nn::MaxPool1dOptions(2).stride(2)); // [batch_size, 32, 50] -> [batch_size, 32, 25]

        // Couches fully connected
        fc1 = register_module("fc1", torch::nn::Linear(6416, 512)); // 16 (state) + 32×25 (asks) + 32×25 (bids) = 1616
        fc2 = register_module("fc2", torch::nn::Linear(512, 256));
        fc3 = register_module("fc3", torch::nn::Linear(256, 128));

        // Têtes pour la politique et la valeur
        policy_head = register_module("policy_head", torch::nn::Linear(128, 3)); // BUY, SELL, WAIT
        value_head = register_module("value_head", torch::nn::Linear(128, 1));

        to(torch::kCUDA);
    }
    catch (const std::exception& e) {
        std::cerr << "Erreur dans TradingAgentNet : " << e.what() << "\n";
        throw;
    }
}


std::pair<torch::Tensor, torch::Tensor> TradingAgentNet::forward(torch::Tensor state, torch::Tensor best_asks, torch::Tensor best_bids) {
    // MLP pour l'état
    auto s = torch::relu(fc_state->forward(state));

    // Convolutions 1D pour asks
    auto asks = torch::relu(bn1d_asks_1->forward(conv1d_asks_1->forward(best_asks)));
    asks = torch::relu(bn1d_asks_2->forward(conv1d_asks_2->forward(asks)));
    asks = pool1d->forward(asks); // [batch_size, 32, 25]
    asks = asks.view({ asks.size(0), -1 }); // [batch_size, 32 × 25 = 800]

    // Convolutions 1D pour bids
    auto bids = torch::relu(bn1d_bids_1->forward(conv1d_bids_1->forward(best_bids)));
    bids = torch::relu(bn1d_bids_2->forward(conv1d_bids_2->forward(bids)));
    bids = pool1d->forward(bids); // [batch_size, 32, 25]
    bids = bids.view({ bids.size(0), -1 }); // [batch_size, 32 × 25 = 800]

    // Concaténation
    auto combined = torch::cat({ s, asks, bids }, 1); // [batch_size, 16 + 800 + 800 = 1616]

    // Couches fully connected
    combined = torch::relu(fc1->forward(combined));
    combined = torch::relu(fc2->forward(combined));
    combined = torch::relu(fc3->forward(combined));

    // Sorties
    auto policy = torch::softmax(policy_head->forward(combined), 1); // Probabilités pour BUY, SELL, WAIT
    auto value = value_head->forward(combined); // Valeur de l'état

    return { policy, value };
}