#include "AI/TradingAI.h"
#include "Heatmap.h"
#include <iostream>
#include <algorithm>

MemoryBuffer::MemoryBuffer(int64_t T_max) : T_max_(T_max), current_size_(0) {
    // Initialiser les tenseurs avec la taille maximale
    best_asks = torch::empty({ T_max, 1, 50 }, torch::dtype(torch::kFloat).device(torch::kCUDA));
    best_bids = torch::empty({ T_max, 1, 50 }, torch::dtype(torch::kFloat).device(torch::kCUDA));
    agent_states = torch::empty({ T_max, 1 }, torch::dtype(torch::kFloat).device(torch::kCUDA));
    actions = torch::empty({ T_max }, torch::dtype(torch::kInt64).device(torch::kCUDA));
    log_probs = torch::empty({ T_max }, torch::dtype(torch::kFloat).device(torch::kCUDA));
    rewards = torch::empty({ T_max }, torch::dtype(torch::kFloat).device(torch::kCUDA));
    values = torch::empty({ T_max }, torch::dtype(torch::kFloat).device(torch::kCUDA));
    dones = torch::empty({ T_max }, torch::dtype(torch::kFloat).device(torch::kCUDA));
}


void MemoryBuffer::store(const Transition& exp) {
    if (current_size_ >= T_max_) {
        throw std::runtime_error("MemoryBuffer is full");
    }
    if (!exp.best_asks.defined() || !exp.best_bids.defined() || !exp.agent_state.defined() ||
        !exp.action.defined() || !exp.log_prob.defined() || !exp.reward.defined() ||
        !exp.value.defined() || !exp.done.defined()) {
        throw std::runtime_error("Invalid tensor in store");
    }
    if (!exp.best_asks.is_cuda() || !exp.best_bids.is_cuda() || !exp.agent_state.is_cuda() ||
        !exp.action.is_cuda() || !exp.log_prob.is_cuda() || !exp.reward.is_cuda() ||
        !exp.value.is_cuda() || !exp.done.is_cuda()) {
        throw std::runtime_error("Tensor not on CUDA in store");
    }
    if (exp.agent_state.dim() != 1 || exp.action.dim() != 1 || exp.log_prob.dim() != 1 ||
        exp.reward.dim() != 1 || exp.value.dim() != 1 || exp.done.dim() != 1) {
        throw std::runtime_error("Transition tensor is not 1D");
    }
    if (exp.best_asks.sizes() != torch::IntArrayRef({ 1, 50 }) ||
        exp.best_bids.sizes() != torch::IntArrayRef({ 1, 50 })) {
        throw std::runtime_error("Best asks/bids shape mismatch: expected [1, 50]");
    }

    best_asks[current_size_] = exp.best_asks.clone().detach();
    best_bids[current_size_] = exp.best_bids.clone().detach();
    agent_states[current_size_] = exp.agent_state.clone().detach();
    actions[current_size_] = exp.action.clone().detach().item<int64_t>();
    log_probs[current_size_] = exp.log_prob.clone().detach().item<float>();
    rewards[current_size_] = exp.reward.clone().detach().item<float>();
    values[current_size_] = exp.value.clone().detach().item<float>();
    dones[current_size_] = exp.done.clone().detach().item<float>();
    current_size_++;
}

void MemoryBuffer::clear() {
    current_size_ = 0;
    
    best_asks.zero_();
    best_bids.zero_();
    agent_states.zero_();
    actions.zero_();
    log_probs.zero_();
    rewards.zero_();
    values.zero_();
    dones.zero_();
}

std::tuple<torch::Tensor, torch::Tensor, torch::Tensor,
    torch::Tensor, torch::Tensor, torch::Tensor, torch::Tensor, torch::Tensor>
    MemoryBuffer::get() const {
    if (current_size_ == 0) {
        return std::make_tuple(
            torch::Tensor(), torch::Tensor(), torch::Tensor(), torch::Tensor(),
            torch::Tensor(), torch::Tensor(), torch::Tensor(), torch::Tensor()
        );
    }
    // Retourner les tranches des tenseurs jusqu'à current_size_
    return std::make_tuple(
        best_asks.slice(0, 0, current_size_),
        best_bids.slice(0, 0, current_size_),
        agent_states.slice(0, 0, current_size_),
        actions.slice(0, 0, current_size_),
        log_probs.slice(0, 0, current_size_),
        rewards.slice(0, 0, current_size_),
        values.slice(0, 0, current_size_),
        dones.slice(0, 0, current_size_)
    );
}

torch::Tensor MemoryBuffer::computeReturns(float last_value, float gamma) const {
    if (current_size_ == 0) {
        return torch::Tensor();
    }

    torch::Tensor rewards_tensor = rewards.slice(0, 0, current_size_);
    torch::Tensor dones_tensor = dones.slice(0, 0, current_size_);
    int64_t T = current_size_;
    torch::Tensor returns = torch::zeros({ T }, torch::dtype(torch::kFloat).device(torch::kCUDA));

    float running_return = last_value;
    for (int64_t t = T - 1; t >= 0; --t) {
        torch::Tensor reward = rewards_tensor[t].detach();
        torch::Tensor done = dones_tensor[t].detach();
        running_return = reward.item<float>() + gamma * running_return * (1.0 - done.item<float>());
        returns[t] = running_return;
    }

    if (T > 1) {
        returns = (returns - returns.mean()) / (returns.std() + 1e-8);
    }
    torch::cuda::synchronize();
    return returns;
}


torch::Tensor MemoryBuffer::computeAdvantages(float last_value, float gamma, float lambda) const {
    if (current_size_ == 0) {
        return torch::Tensor();
    }

    torch::Tensor rewards_tensor = rewards.slice(0, 0, current_size_);
    torch::Tensor values_tensor = values.slice(0, 0, current_size_);
    torch::Tensor dones_tensor = dones.slice(0, 0, current_size_);
    int64_t T = current_size_;
    torch::Tensor advantages = torch::zeros({ T }, torch::dtype(torch::kFloat).device(torch::kCUDA));

    float delta = 0.0;
    float advantage = 0.0;
    float next_value = last_value;

    for (int64_t t = T - 1; t >= 0; --t) {
        torch::Tensor reward = rewards_tensor[t].detach();
        torch::Tensor value = values_tensor[t].detach();
        torch::Tensor done = dones_tensor[t].detach();
        delta = reward.item<float>() + gamma * next_value * (1.0 - done.item<float>()) - value.item<float>();
        advantage = delta + gamma * lambda * advantage * (1.0 - done.item<float>());
        advantages[t] = advantage;
        next_value = value.item<float>();
    }

    if (T > 1) {
        advantages = (advantages - advantages.mean()) / (advantages.std() + 1e-8);
    }
    torch::cuda::synchronize();
    return advantages;
}


Transition MemoryBuffer::sampleMiniBatch(int batch_size, const torch::Tensor& indices) const {
    if (current_size_ == 0) {
        return Transition{
            torch::Tensor(), torch::Tensor(), torch::Tensor(),
            torch::Tensor(), torch::Tensor(), torch::Tensor(),
            torch::Tensor(), torch::Tensor()
        };
    }

    auto sliced = [&](const torch::Tensor& t) {
        return t.slice(0, 0, current_size_).index_select(0, indices).to(torch::kCUDA);
        };

    return Transition{
        sliced(best_asks),
        sliced(best_bids),
        sliced(agent_states),
        sliced(actions),
        sliced(log_probs),
        sliced(rewards),
        sliced(values),
        sliced(dones)
    };
}


// --------------------- TRADING ENVIRONMENT FOR TRAINING ---------------------


TradingEnvironment::TradingEnvironment(TradingAgentNet* network, 
    std::mt19937 rng, int N_trajectories,
    int traj_duration, int decision_per_second) : orderBook(), network(network), 
    traj_duration(traj_duration), decision_per_second(decision_per_second), 
    memoryBuffer(traj_duration * decision_per_second), heatmap(0, 50), rng(rng)

{
    optimizer = new torch::optim::Adam(network->parameters(), torch::optim::AdamOptions().lr(1e-4));
 
    std::cout << "filling up the heatmap matrix..." << "\n";
    orderBook.initialize_book();
    orderBook.setInitialLiquidity(500, rng);
    
    for (int i = 0; i < 50; ++i) {
        orderBook.update(marketUpdatePerDecision, rng);
        heatmap.updateData(orderBook.getCurrentBook());

    }
    best_asks_tensor = torch::from_blob(heatmap.best_ask_history.data(), { 1, 1, 50 },
        torch::kFloat).to(torch::kCUDA);
    best_bids_tensor = torch::from_blob(heatmap.best_bid_history.data(), { 1, 1, 50 },
        torch::kFloat).to(torch::kCUDA);
}

TradingEnvironment::~TradingEnvironment() { delete optimizer; }

void TradingEnvironment::reset() {
    current_decision_index = 0;
    isEpisodeDone = false;
    reward_windows.clear();
    entry_price = 0.0f;
    agent_state.position = 0;
    open_trade.reset();
    trade_logs.clear(); 
    memoryBuffer.clear(); 

    orderBook.initialize_book();
    orderBook.setInitialLiquidity(500, rng);

    for (int i = 0; i < 50; ++i) {
        orderBook.update(marketUpdatePerDecision, rng);
        heatmap.updateData(orderBook.getCurrentBook());

    }
    best_asks_tensor = torch::from_blob(heatmap.best_ask_history.data(), { 1, 1, 50 },
        torch::kFloat).to(torch::kCUDA);
    best_bids_tensor = torch::from_blob(heatmap.best_bid_history.data(), { 1, 1, 50 },
        torch::kFloat).to(torch::kCUDA);
}

Action TradingEnvironment::sampleFromPolicy(const torch::Tensor& policy,
	std::mt19937& rng) {

    return static_cast<Action>(torch::multinomial(policy, 1, true).item<int64_t>());
}

void TradingEnvironment::handleAction(Action action, const torch::Tensor& policy, const torch::Tensor& value) {
    int current_timestep = current_decision_index;
    float best_bid = orderBook.getCurrentBestBid();
    float best_ask = orderBook.getCurrentBestAsk();
    int current_position = agent_state.position;

    torch::Tensor state_tensor = agent_state.toTensor(); // [1]
    torch::Tensor best_asks_fot_storage = best_asks_tensor.squeeze(1);
    torch::Tensor best_bids_fot_storage = best_bids_tensor.squeeze(1);

    float commission = 4.58f;
    float leverage = 20.0f;

    torch::Tensor log_prob = torch::log(policy[0][static_cast<int64_t>(action)])
        .unsqueeze(0).to(torch::kCUDA);


    if (action == Action::BUY_MARKET) {
        //std::cout << "BUY" << "\n";
        if (agent_state.position == 0) {
            //std::cout << "long opened" << "\n";
            agent_state.position = 1;
            entry_price = best_ask;

            open_trade = TradeLog{
                current_timestep,
                entry_price,
                -1, // pas encore fermé
                0.0f,
                0.0f,
                Action::BUY_MARKET,
                Action::WAIT
            };
            MarketOrder order = orderBook.generateMarketOrder();
            order.side = Side::ASK;
            order.size = 1;
            orderBook.processMarketOrder(order);
            //std::cout << "position agent: " << agent_state.position << "\n";
        }
        else if (agent_state.position == -1) {
            float exit_price = best_ask;
            float pnl = entry_price - exit_price;
            pnl = pnl * leverage - commission;

            if (open_trade.has_value()) {
                //std::cout << "short closed" << "\n";
                open_trade->timestep_exit = current_timestep;
                open_trade->price_exit = exit_price;
                open_trade->pnl = pnl;
                open_trade->exit_action = Action::BUY_MARKET;
                trade_logs.push_back(open_trade.value());
                open_trade.reset();
            }
            agent_state.position = 0;
            MarketOrder order = orderBook.generateMarketOrder();
            order.side = Side::ASK;
            order.size = 1;
            orderBook.processMarketOrder(order);
            entry_price = best_ask;
        }
    }
    else if (action == Action::SELL_MARKET) {
        //std::cout << "SELL" << "\n";
        if (agent_state.position == 0) {
            //std::cout << "short opened" << "\n";
            agent_state.position = -1;
            entry_price = best_bid;

            open_trade = TradeLog{
                current_timestep,
                entry_price,
                -1,
                0.0f,
                0.0f,
                Action::SELL_MARKET,
                Action::WAIT
            };
            MarketOrder order = orderBook.generateMarketOrder();
            order.side = Side::BID;
            order.size = 1;
            orderBook.processMarketOrder(order);
            //std::cout << "position agent: " << agent_state.position << "\n";
        }
        else if (agent_state.position == 1) {
            float exit_price = best_bid;
            float pnl = exit_price - entry_price;
            pnl = pnl * leverage - commission;

            if (open_trade.has_value()) {
                //std::cout << "long closed" << "\n";
                open_trade->timestep_exit = current_timestep;
                open_trade->price_exit = exit_price;
                open_trade->pnl = pnl;
                open_trade->exit_action = Action::SELL_MARKET;
                trade_logs.push_back(open_trade.value());
                open_trade.reset();
            }
            agent_state.position = 0;
            MarketOrder order = orderBook.generateMarketOrder();
            order.side = Side::BID;
            order.size = 1;
            orderBook.processMarketOrder(order);
            entry_price = best_bid;
        }
    }

    if (action == Action::WAIT) {
        if (agent_state.position == 1) {
            entry_price = best_ask;
        }
        else if (agent_state.position == -1) {
            entry_price = best_bid;
        }
    }


    bool isInvalid = false;
    if ((action == Action::BUY_MARKET && current_position == 1) ||
        (action == Action::SELL_MARKET && current_position == -1)) {

        isInvalid = true;
    }

 
    RewardWindow rw(current_decision_index, action, entry_price, current_position,
        policy[0][static_cast<int64_t>(action)].unsqueeze(0).to(torch::kCUDA),
        value, state_tensor, best_asks_fot_storage,
        best_bids_fot_storage, isInvalid);

    reward_windows.push_back(rw);
}


void TradingEnvironment::updateRewardWindows() {
    float best_bid = orderBook.getCurrentBestBid();
    float best_ask = orderBook.getCurrentBestAsk();

    for (auto it = reward_windows.begin(); it != reward_windows.end();) {
        it->addPnL(best_bid, best_ask);

        if (it->isComplete()) {
            torch::Tensor reward = it->computeWeightedReward();
            torch::Tensor action_tensor = torch::tensor({ static_cast<int64_t>(it->action) },
                torch::kInt64).to(torch::kCUDA);
            torch::Tensor log_prob = torch::log(it->proba);
            torch::Tensor done_tensor = torch::tensor({ isEpisodeDone ? 1.0f : 0.0f },
                torch::kFloat).to(torch::kCUDA);
            Transition transition{ it->best_asks, it->best_bids,
                                it->agent_state_tensor, action_tensor,
                                log_prob, reward, it->value.squeeze(0), done_tensor };
            memoryBuffer.store(transition);

            it = reward_windows.erase(it);
        }
        else {
            ++it;
        }
    }
}


void TradingEnvironment::printTradeLogs() const {
    std::cout << "Trade history (" << trade_logs.size() << " trades):\n";
    for (const auto& trade : trade_logs) {
        std::cout
            << "Entry t=" << trade.timestep_entry
            << " price=" << trade.price_entry
            << " | Exit t=" << trade.timestep_exit
            << " price=" << trade.price_exit
            << " | PnL=" << trade.pnl <<" $"
            << " | EntryAction=" << trade.entry_action
            << " ExitAction=" << trade.exit_action
            << "\n";
    }
}

void TradingEnvironment::collectTransitions(std::mt19937& rng) {
    int total_steps = traj_duration * decision_per_second;
    int bar_width = 20;
    const int display_interval = total_steps / bar_width;

    for (int i = 0; i < traj_duration * decision_per_second; ++i) {
        // Update order book
        orderBook.update(marketUpdatePerDecision, rng);
        float best_bid = orderBook.getCurrentBestBid();
        float best_ask = orderBook.getCurrentBestAsk();
        
        if (best_ask >= 20045.0f || best_bid <= 19955.0f) {
            std::cout << "[INFO] Trajectory interrupted: price out of bounds. "
                << "best_ask=" << best_ask << ", best_bid=" << best_bid << "\n";
            isEpisodeDone = true;
            break;
        }

        // ---------- Console Debugging ----------
        if (i % display_interval == 0 || i == total_steps - 1) {
            float progress = static_cast<float>(i + 1) / total_steps;
            int pos = static_cast<int>(bar_width * progress);

            std::cout << "\rCollecting transitions: [";
            for (int j = 0; j < bar_width; ++j)
                std::cout << (j < pos ? "#" : "-");
            std::cout << "] " << int(progress * 100.0) << "%" << std::flush;
        }
        // ---------------------------------------

        // Update heatmap and agent_state
        heatmap.updateData(orderBook.getCurrentBook());

        best_asks_tensor = torch::from_blob(heatmap.best_ask_history.data(), { 1, 1, 50 },
            torch::kFloat).to(torch::kCUDA);
        best_bids_tensor = torch::from_blob(heatmap.best_bid_history.data(), { 1, 1, 50 },
            torch::kFloat).to(torch::kCUDA);
        torch::Tensor state_tensor = agent_state.toTensor().unsqueeze(0);
        // Policy and value
        
        auto [policy, value] = network->forward(state_tensor, best_asks_tensor, best_bids_tensor);
        //std::cout << "  Policy: [" << policy[0][0].item<float>() << ", " << policy[0][1].item<float>() << ", " << policy[0][2].item<float>() << "]" << std::endl;
        //std::cout << "  Value: " << value.item<float>() << std::endl;

        // Sample action
        Action action = sampleFromPolicy(policy, rng);
        //std::cout << "  Action sampled: " << static_cast<int>(action);
        //switch (action) {
        //case Action::BUY_MARKET: std::cout << " (BUY_MARKET)"; break;
        //case Action::SELL_MARKET: std::cout << " (SELL_MARKET)"; break;
        //case Action::WAIT: std::cout << " (WAIT)"; break;
        //}
        //std::cout << std::endl;

        handleAction(action, policy, value);
        updateRewardWindows();

        current_decision_index++;
        //std::cout << "  MemoryBuffer size: " << std::get<0>(memoryBuffer.get()).size(0) << std::endl;
        //std::cout << "-----------------------------" << std::endl;

        // Check if trajectory is done
        if (current_decision_index >= traj_duration * decision_per_second) {
            isEpisodeDone = true;
            break;
        }
    }

    // Print transitions
    /*const auto& transitions = memoryBuffer.get();
    int64_t num_transitions = std::get<0>(transitions).size(0);
    for (int64_t i = 0; i < num_transitions; ++i) {
        std::cout << "Transition #" << i << std::endl;
        torch::Tensor action = std::get<2>(transitions).index({ i });
        std::cout << "  Action       : " << action.item<int64_t>() << " (";
        switch (action.item<int64_t>()) {
        case 0: std::cout << "BUY_MARKET"; break;
        case 1: std::cout << "SELL_MARKET"; break;
        case 2: std::cout << "WAIT"; break;
        }
        std::cout << ")" << std::endl;
        std::cout << "  LogProb      : " << std::get<3>(transitions).index({ i }).item<float>() << std::endl;
        std::cout << "  Value        : " << std::get<5>(transitions).index({ i }).item<float>() << std::endl;
        std::cout << "  Reward       : " << std::get<4>(transitions).index({ i }).item<float>() << std::endl;
        std::cout << "  Done         : " << std::get<6>(transitions).index({ i }).item<float>() << std::endl;
        std::cout << "-----------------------------" << std::endl;
    }*/

    // Afficher l'historique des trades
    //std::cout << "=== Trade history ===" << std::endl;
    //printTradeLogs();
    std::cout << "\n";
}

void TradingEnvironment::optimize(std::mt19937& rng, int num_epochs, int batch_size,
    float clip_param, float value_loss_coef, float entropy_coef) {

    const auto& transitions = memoryBuffer.get();
    int64_t num_transitions = std::get<0>(transitions).size(0);
    if (num_transitions == 0) {
        std::cout << "No transitions to optimize" << std::endl;
        return;
    }
    int64_t num_batches = (num_transitions + batch_size - 1) / batch_size;
    // Compute Advantages and Returns
    float last_value = 0.0f;
    float gamma = 0.99f;
    float lambda = 0.95f;
    torch::Tensor advantages = memoryBuffer.computeAdvantages(last_value, gamma, lambda);
    torch::Tensor returns = memoryBuffer.computeReturns(last_value, gamma);

    // Optimization loop
    for (int epoch = 1; epoch < num_epochs + 1; ++epoch) {
        std::cout << "\rOptimizing: [" << (epoch) << " / " << num_epochs << "]" << std::flush;

        auto all_indices = torch::randperm(num_transitions, torch::TensorOptions().dtype(torch::kInt64)).to(torch::kCUDA);

        for (int64_t batch_idx = 0; batch_idx < num_batches; ++batch_idx) {
            //Sample minibatch
            auto start = batch_idx * batch_size;
            auto end = std::min(start + batch_size, num_transitions);
            auto selected_indices = all_indices.slice(0, start, end);

            Transition batch = memoryBuffer.sampleMiniBatch(batch_size, selected_indices);

            // Extract Advantages and Returns for that specific mini-batch
            auto batch_advantages = advantages.index_select(0, selected_indices);
            auto batch_returns = returns.index_select(0, selected_indices);

            // Compute new policy and value
            auto [policy, value] = network->forward(batch.agent_state, batch.best_asks, batch.best_bids);
            auto log_probs = torch::log(policy.gather(1, batch.action.unsqueeze(1))).squeeze(1);
            auto ratios = torch::exp(log_probs - batch.log_prob);

            // Perte PPO
            auto surr1 = ratios * batch_advantages;
            auto surr2 = torch::clamp(ratios, 1.0f - clip_param, 1.0f + clip_param) * batch_advantages;
            auto policy_loss = -torch::min(surr1, surr2).mean();

            // Perte de valeur
            auto value_loss = torch::mse_loss(value.squeeze(1), batch_returns);

            // Entropie
            auto entropy = -(policy * torch::log(policy + 1e-10)).sum(1).mean();

            // Perte totale
            auto loss = policy_loss + value_loss_coef * value_loss - entropy_coef * entropy;

            // Backpropagation
            optimizer->zero_grad();
            loss.backward();
            optimizer->step();

            // Logs
            /*std::cout << "  Batch " << batch_idx << "/" << num_batches
                << ": Policy loss = " << policy_loss.item<float>()
                << ", Value loss = " << value_loss.item<float>()
                << ", Entropy = " << entropy.item<float>()
                << ", Total loss = " << loss.item<float>() << std::endl;*/
        }
    }
    std::cout << "\n";
}


void TradingEnvironment::train(int num_trajectories, int num_epochs, int batch_size, float clip_param, float value_loss_coef, float entropy_coef) {
    std::filesystem::remove("C:/Users/Ilan/VisualStudioProjects/BookMap-mk1/assets/metrics.csv"); // Supprime le fichier s'il existe
    std::ofstream metrics_file("C:/Users/Ilan/VisualStudioProjects/BookMap-mk1/assets/metrics.csv", std::ios::out);
    if (metrics_file.is_open()) {
        metrics_file << "Episode,Total_PnL,Max_Drawdown,Sharpe_Ratio,Num_Trades\n";
        metrics_file.close();
    }

    std::filesystem::remove("C:/Users/Ilan/VisualStudioProjects/BookMap-mk1/assets/trades.csv"); // Supprime le fichier s'il existe
    std::ofstream trades_file("C:/Users/Ilan/VisualStudioProjects/BookMap-mk1/assets/trades.csv", std::ios::out);
    if (trades_file.is_open()) {
        trades_file << "Episode,Timestep_Entry,Price_Entry,Timestep_Exit,Price_Exit,PnL,Entry_Action,Exit_Action\n";
        trades_file.close();
    }

    std::cout << "=== Training for " << num_trajectories << " trajectories ===" << std::endl;
    for (int episode = 1; episode < num_trajectories + 1; ++episode) {
        std::cout << "=== Episode " << episode << " ===" << std::endl;
        reset();
        collectTransitions(rng);
        optimize(rng, num_epochs, batch_size, clip_param, value_loss_coef, entropy_coef);
        computeMetrics(episode);

    }
    std::cout << "=== Training completed ===" << std::endl;
    torch::cuda::synchronize();
}



void TradingEnvironment::computeMetrics(int episode) {
    if (trade_logs.empty()) {
        std::cout << "No trades recorded" << std::endl;
        return;
    }

    float total_pnl = 0.0f;
    float max_drawdown = 0.0f;
    float running_pnl = 0.0f;
    std::vector<float> pnls;
    for (const auto& trade : trade_logs) {
        total_pnl += trade.pnl;
        running_pnl += trade.pnl;
        max_drawdown = std::min(max_drawdown, running_pnl);
        pnls.push_back(trade.pnl);
    }

    // Calculer le Sharpe ratio
    float mean_pnl = total_pnl / std::max(1, static_cast<int>(pnls.size()));
    float variance = 0.0f;
    for (float pnl : pnls) {
        variance += (pnl - mean_pnl) * (pnl - mean_pnl);
    }
    variance /= std::max(1, static_cast<int>(pnls.size()));
    float sharpe = variance > 0.0f ? mean_pnl / (std::sqrt(variance) + 1e-8) : 0.0f;

    //std::cout << "Number of trades: " << trade_logs.size() << std::endl;
    //std::cout << "Total PnL: " << total_pnl << " $" << std::endl;
    //std::cout << "Max Drawdown: " << max_drawdown << " $" << std::endl;
    //std::cout << "Sharpe Ratio: " << sharpe << std::endl;

    std::ofstream metrics_file("C:/Users/Ilan/VisualStudioProjects/BookMap-mk1/assets/metrics.csv", std::ios::app);
    if (metrics_file.is_open()) {
        metrics_file << std::fixed << std::setprecision(2);
        metrics_file << episode << "," << total_pnl << "," << max_drawdown << "," << sharpe << "," << trade_logs.size() << "\n";
        metrics_file.close();
    }
    else {
        std::cerr << "Error: Could not append to metrics.csv" << std::endl;
    }


    std::ofstream trades_file("C:/Users/Ilan/VisualStudioProjects/BookMap-mk1/assets/trades.csv", std::ios::app);
    if (trades_file.is_open()) {
        for (const auto& trade : trade_logs) {
            trades_file << episode << "," << trade.timestep_entry << "," << trade.price_entry << ","
                << trade.timestep_exit << "," << trade.price_exit << "," << trade.pnl << ","
                << static_cast<int>(trade.entry_action) << "," << static_cast<int>(trade.exit_action) << "\n";
        }
        trades_file.close();
    }
    else {
        std::cerr << "Error: Could not append to trades.csv" << std::endl;
    }
}