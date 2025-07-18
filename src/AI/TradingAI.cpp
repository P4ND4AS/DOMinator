#include "AI/TradingAI.h"
#include "Heatmap.h"
#include <iostream>
#include <algorithm>

MemoryBuffer::MemoryBuffer(int64_t T_max) : T_max_(T_max), current_size_(0) {
    // Initialiser les tenseurs avec la taille maximale
    heatmaps = torch::empty({ T_max, 1, 401, 800 }, torch::dtype(torch::kFloat).device(torch::kCUDA));
    agent_states = torch::empty({ T_max }, torch::dtype(torch::kFloat).device(torch::kCUDA));
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
    if (!exp.heatmap.defined() || !exp.agent_state.defined() || !exp.action.defined() ||
        !exp.log_prob.defined() || !exp.reward.defined() || !exp.value.defined() || !exp.done.defined()) {
        throw std::runtime_error("Invalid tensor in store");
    }
    if (!exp.heatmap.is_cuda() || !exp.agent_state.is_cuda() || !exp.action.is_cuda() ||
        !exp.log_prob.is_cuda() || !exp.reward.is_cuda() || !exp.value.is_cuda() || !exp.done.is_cuda()) {
        throw std::runtime_error("Tensor not on CUDA in store");
    }
    if (exp.agent_state.dim() != 1 || exp.action.dim() != 1 || exp.log_prob.dim() != 1 ||
        exp.reward.dim() != 1 || exp.value.dim() != 1 || exp.done.dim() != 1) {
        throw std::runtime_error("Transition tensor is not 1D");
    }
    if (exp.heatmap.sizes() != torch::IntArrayRef({ 1, 401, 800 })) {
        throw std::runtime_error("Heatmap shape mismatch: expected [1, 401, 800], got " +
            std::to_string(exp.heatmap.sizes()[0]) + "," +
            std::to_string(exp.heatmap.sizes()[1]) + "," +
            std::to_string(exp.heatmap.sizes()[2]));
    }

    heatmaps[current_size_] = exp.heatmap.clone().detach();
    agent_states[current_size_] = exp.agent_state.clone().detach().item<float>();
    actions[current_size_] = exp.action.clone().detach().item<int64_t>();
    log_probs[current_size_] = exp.log_prob.clone().detach().item<float>();
    rewards[current_size_] = exp.reward.clone().detach().item<float>();
    values[current_size_] = exp.value.clone().detach().item<float>();
    dones[current_size_] = exp.done.clone().detach().item<float>();
    current_size_++;
}

void MemoryBuffer::clear() {
    current_size_ = 0;
    
    heatmaps.zero_();
    agent_states.zero_();
    actions.zero_();
    log_probs.zero_();
    rewards.zero_();
    values.zero_();
    dones.zero_();
}

std::tuple<torch::Tensor, torch::Tensor, torch::Tensor, torch::Tensor, 
    torch::Tensor, torch::Tensor, torch::Tensor> MemoryBuffer::get() const {
    if (current_size_ == 0) {
        return std::make_tuple(
            torch::Tensor(), torch::Tensor(), torch::Tensor(),
            torch::Tensor(), torch::Tensor(), torch::Tensor(), torch::Tensor()
        );
    }
    // Retourner les tranches des tenseurs jusqu'à current_size_
    return std::make_tuple(
        heatmaps.slice(0, 0, current_size_),
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


std::tuple<torch::Tensor, torch::Tensor, torch::Tensor, torch::Tensor, torch::Tensor, torch::Tensor, torch::Tensor>
MemoryBuffer::sampleMiniBatch(int batch_size, std::mt19937& rng) const {
    if (current_size_ == 0) {
        return std::make_tuple(
            torch::Tensor(), torch::Tensor(), torch::Tensor(),
            torch::Tensor(), torch::Tensor(), torch::Tensor(), torch::Tensor()
        );
    }

    // Créer indices et mélanger
    std::vector<int64_t> indices(current_size_);
    std::iota(indices.begin(), indices.end(), 0);
    std::shuffle(indices.begin(), indices.end(), rng);

    // Prendre les batch_size premiers indices
    int64_t batch_size_actual = std::min(static_cast<int64_t>(batch_size), current_size_);
    auto selected_indices = torch::tensor(indices).slice(0, 0, batch_size_actual).to(torch::kCUDA);

    // Retourner les tranches indexées
    return std::make_tuple(
        heatmaps.slice(0, 0, current_size_).index_select(0, selected_indices),
        agent_states.slice(0, 0, current_size_).index_select(0, selected_indices),
        actions.slice(0, 0, current_size_).index_select(0, selected_indices),
        log_probs.slice(0, 0, current_size_).index_select(0, selected_indices),
        rewards.slice(0, 0, current_size_).index_select(0, selected_indices),
        values.slice(0, 0, current_size_).index_select(0, selected_indices),
        dones.slice(0, 0, current_size_).index_select(0, selected_indices)
    );
}


// --------------------- TRADING ENVIRONMENT FOR TRAINING ---------------------

/*
TradingEnvironment::TradingEnvironment(OrderBook* book, PolicyValueNet* network)
	: orderBook(book), heatmap(128, 128), network(network)
{
    std::cout << "Environment created" << "\n";
    std::cout << "Shape of heatmap data : " << heatmap.data.rows() << "x" << heatmap.data.cols() << "\n\n";
    if (!network) {
        std::cerr << "Erreur: network est nullptr!" << std::endl;
        return;
    }
}


Action TradingEnvironment::sampleFromPolicy(const Eigen::VectorXf& policy,
	std::mt19937& rng) {
	if (policy.size() != 3) { throw std::invalid_argument("Policy must have 3 elements"); }

	std::discrete_distribution<int> dist(policy.begin(), policy.end());
	int action_index = dist(rng);
	return static_cast<Action>(action_index);
}

void TradingEnvironment::handleAction(Action action, const Eigen::VectorXf& policy, const float value) {
    int current_timestep = current_decision_index;
    float bestbid = orderBook->getCurrentBestBid();
    float bestask = orderBook->getCurrentBestAsk();

    int currentPosition = agent_state.position;

    if (action == BUY_MARKET) {
        std::cout << "BUY" << "\n";
        if (agent_state.position == 0) {
            std::cout<<"long opened"<<"\n";
            agent_state.position = 1;
            entry_price = bestask;

            open_trade = TradeLog{
                current_timestep,
                entry_price,
                -1,  // pas encore fermé
                0.0f,
                0.0f,
                BUY_MARKET,
                WAIT  
            };
            MarketOrder order = orderBook->generateMarketOrder();
            order.side = Side::ASK;
            order.size = 1;
            orderBook->processMarketOrder(order);
            std::cout << "position agent: " << agent_state.position << "\n";
        }
        else if (agent_state.position == -1) {
            float exit_price = bestask;
            float pnl = entry_price - exit_price;

            if (open_trade.has_value()) {
                std::cout << "short closed" << "\n";
                open_trade->timestep_exit = current_timestep;
                open_trade->price_exit = exit_price;
                open_trade->pnl = pnl;
                open_trade->exit_action = BUY_MARKET;
                trade_logs.push_back(open_trade.value());
                open_trade.reset();
            }
            agent_state.position = 0;
            MarketOrder order = orderBook->generateMarketOrder();
            order.side = Side::ASK;
            order.size = 1;
            orderBook->processMarketOrder(order);
            entry_price = bestask;
        }
    }
    else if (action == SELL_MARKET) {
        std::cout << "SELL" << "\n";
        if (agent_state.position == 0) {
            std::cout << "short opened" << "\n";
            agent_state.position = -1;
            entry_price = bestbid;

            open_trade = TradeLog{
                current_timestep,
                entry_price,
                -1,
                0.0f,
                0.0f,
                SELL_MARKET,
                WAIT
            };
            MarketOrder order = orderBook->generateMarketOrder();
            order.side = Side::BID;
            order.size = 1;
            orderBook->processMarketOrder(order);
        }
        else if (agent_state.position == 1) {
            float exit_price = bestbid;
            float pnl = exit_price - entry_price;

            if (open_trade.has_value()) {
                std::cout << "long closed" << "\n";
                open_trade->timestep_exit = current_timestep;
                open_trade->price_exit = exit_price;
                open_trade->pnl = pnl;
                open_trade->exit_action = SELL_MARKET;
                trade_logs.push_back(open_trade.value());
                open_trade.reset();
            }
            agent_state.position = 0;
            MarketOrder order = orderBook->generateMarketOrder();
            order.side = Side::BID;
            order.size = 1;
            orderBook->processMarketOrder(order);
            entry_price = bestbid;
        }
    }

    if (action == WAIT) {
        if (currentPosition == 1) {
            entry_price = bestask;
        }
        else if (currentPosition == -1) {
            entry_price = bestbid;
        }
    }


    bool isInvalid = false;
    if ((action == BUY_MARKET && currentPosition == 1) ||
        (action == SELL_MARKET && currentPosition == -1)) {

        isInvalid = true;
    }

    RewardWindow rw(current_decision_index, action, entry_price, currentPosition, policy[action], value, isInvalid);
    reward_windows.push_back(rw);
}


void TradingEnvironment::updateRewardWindows() {

	for (auto it = reward_windows.begin(); it != reward_windows.end();) {
		it->addPnL(orderBook->getCurrentBestBid(), orderBook->getCurrentBestAsk());

		if (it->isComplete()) {
			float reward = it->computeWeightedReward();

			Transition transition;
			transition.action = it->action;
			transition.log_prob = std::log(it->proba);
			transition.reward = reward;
			transition.value = it->value;
			transition.done = isEpisodeDone;

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
            << " | PnL=" << trade.pnl
            << " | EntryAction=" << trade.entry_action
            << " ExitAction=" << trade.exit_action
            << "\n";
    }
}


void TradingEnvironment::train(std::mt19937& rng) {
    Eigen::MatrixXf dummy_heatmap = Eigen::MatrixXf::Zero(401, 128);
    for (int traj = 0; traj < N_trajectories; ++traj) {
        std::cout << "Trajectory #" << traj << "\n\n";
        
        std::cout << "start updating market" << "\n";
        for (int iter = 0; iter < 128; ++iter) {
            orderBook->update(20000, rng);
            heatmap.updateData(orderBook->getCurrentBook());
        }
        std::cout << "OrderBook sped up a few seconds..." << "\n";

        std::cout << "marketUpdatePerDecision : " << marketUpdatePerDecision << "\n";

        for (int iter = 0; iter < traj_duration * decision_per_second; ++iter) {
            std::cout << "Iteration #" << iter << "\n\n";
            orderBook->update(20000, rng);
            std::cout << "BestBid : " << orderBook->getCurrentBestBid() << " & BestAsk : " << orderBook->getCurrentBestAsk() << "\n";
            heatmap.updateData(orderBook->getCurrentBook());
            std::cout << "Matrice updated" << "\n";
            auto [policy, value] = network->forward(heatmap.data, agent_state.toVector());
           
            std::cout << "Policy : " << policy << " & value : " << value << "\n";

            Action action = sampleFromPolicy(policy, rng);
            std::cout << "Action : " << action << "\n";
            handleAction(action, policy, value);
            std::cout << "Action handled" << "\n";
            updateRewardWindows();
            std::cout << "\n\n";

            ++current_decision_index;

        }
        printTradeLogs();

        auto [_, lastValue] = network->forward(heatmap.data, agent_state.toVector());
        std::vector<float> advantages = memoryBuffer.computeAdvantages(lastValue);
        std::vector<float> returns = memoryBuffer.computeReturns(lastValue);

        std::cout << "=== Transitions enregistrées ===" << std::endl;
        for (size_t i = 0; i < memoryBuffer.get().size(); ++i) {
            const auto& t = memoryBuffer.get()[i];
            std::cout << "Transition #" << i << std::endl;
            std::cout << "  Action       : " << t.action << std::endl;
            std::cout << "  LogProb      : " << t.log_prob << std::endl;
            std::cout << "  Value        : " << t.value << std::endl;
            std::cout << "  Reward       : " << t.reward << std::endl;
            std::cout << "  Done         : " << t.done << std::endl;
            std::cout << "  Advantage    : " << advantages[i] << std::endl;
            std::cout << "  Return       : " << returns[i] << std::endl;
            std::cout << "-----------------------------" << std::endl;
        }
            memoryBuffer.clear();
    }
}*/