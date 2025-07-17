#include "AI/TradingAI.h"
#include "Heatmap.h"
#include <iostream>
#include <algorithm>


void MemoryBuffer::store(const Transition& exp) {
    heatmaps.push_back(exp.heatmap.clone().detach().to(torch::kCUDA));
    agent_states.push_back(exp.agent_state.clone().detach().to(torch::kCUDA));
    actions.push_back(exp.action.clone().detach().to(torch::kCUDA));
    log_probs.push_back(exp.log_prob.clone().detach().to(torch::kCUDA));
    rewards.push_back(exp.reward.clone().detach().to(torch::kCUDA));
    values.push_back(exp.value.clone().detach().to(torch::kCUDA));
    dones.push_back(exp.done.clone().detach().to(torch::kCUDA));
}

void MemoryBuffer::clear() {
    heatmaps.clear();
    agent_states.clear();
    actions.clear();
    log_probs.clear();
    rewards.clear();
    values.clear();
    dones.clear();
}

std::tuple<torch::Tensor, torch::Tensor, torch::Tensor, torch::Tensor, torch::Tensor, 
    torch::Tensor, torch::Tensor> MemoryBuffer::get() const {
    // Concaténer les tenseurs pour obtenir un batch
    auto heatmap_tensor = torch::cat(heatmaps, 0);
    auto agent_state_tensor = torch::cat(agent_states, 0);
    auto action_tensor = torch::cat(actions, 0);
    auto log_prob_tensor = torch::cat(log_probs, 0);
    auto reward_tensor = torch::cat(rewards, 0);
    auto value_tensor = torch::cat(values, 0);
    auto done_tensor = torch::cat(dones, 0);

    return std::make_tuple(
        heatmap_tensor,
        agent_state_tensor,
        action_tensor,
        log_prob_tensor,
        reward_tensor,
        value_tensor,
        done_tensor
    );
}

torch::Tensor MemoryBuffer::computeAdvantages(float last_value, float gamma, float lambda) const {
    if (rewards.empty()) {
        return torch::Tensor();
    }

    torch::Tensor rewards = torch::cat(rewards, 0).to(torch::kCUDA);
    torch::Tensor values = torch::cat(values, 0).to(torch::kCUDA);
    torch::Tensor dones = torch::cat(dones, 0).to(torch::kCUDA);
    int64_t T = rewards.size(0);
    torch::Tensor advantages = torch::zeros({ T }).to(torch::kCUDA);

    float delta = 0.0;
    float advantage = 0.0;
    float next_value = last_value;

    for (int64_t t = T - 1; t >= 0; --t) {
        torch::Tensor reward = rewards[t].detach();
        torch::Tensor value = values[t].detach();
        torch::Tensor done = dones[t].detach();
        if (!reward.is_nonzero() || !value.is_nonzero() || !done.is_nonzero()) {
            throw std::runtime_error("Invalid tensor access at index " + std::to_string(t));
        }
        delta = reward.item<float>() + gamma * next_value * (1.0 - done.item<float>()) - value.item<float>();
        advantage = delta + gamma * lambda * advantage * (1.0 - done.item<float>());
        advantages[t] = advantage;
        next_value = value.item<float>();
    }

    // Normaliser les avantages
    if (T > 1) {
        advantages = (advantages - advantages.mean()) / (advantages.std() + 1e-8);
    }
    return advantages;
}


torch::Tensor MemoryBuffer::computeReturns(float last_value, float gamma) const {
    if (rewards.empty()) {
        std::cout << "'rewards' is empty" << "\n";
        return torch::Tensor();
    }
    std::cout << "'rewards' to be concatenated" << "\n";
    torch::Tensor rewards = torch::cat(rewards, 0).to(torch::kCUDA);
    std::cout << "'rewards' concatenated" << "\n";
    torch::Tensor dones = torch::cat(dones, 0).to(torch::kCUDA);
    std::cout << "'dones' concatenated" << "\n";
    int64_t T = rewards.size(0);
    torch::Tensor returns = torch::zeros({ T }).to(torch::kCUDA);
    std::cout << "'returns' initialized" << "\n";

    // Calcul vectorisé des retours
    float running_return = last_value;
    for (int64_t t = T - 1; t >= 0; --t) {
        // Vérifier que les tenseurs sont accessibles
        torch::Tensor reward = rewards[t].detach();
        std::cout << "reward tensor detached: " << reward << "\n";
        torch::Tensor done = dones[t].detach();
        std::cout << "done tensor detached: " << reward << "\n";

        running_return = reward.item<float>() + gamma * running_return * (1.0 - done.item<float>());
        std::cout << "running_return calculated " << "\n";
        returns[t] = running_return;
    }

    // Normaliser les retours
    if (T > 1) {
        std::cout << "Normalized returns " << "\n";
        returns = (returns - returns.mean()) / (returns.std() + 1e-8);
    }
    return returns;
}


std::tuple<torch::Tensor, torch::Tensor, torch::Tensor, torch::Tensor, torch::Tensor, torch::Tensor, torch::Tensor>
MemoryBuffer::sampleMiniBatch(int batch_size, std::mt19937& rng) const {
    auto [heatmaps, agent_states, actions, log_probs, rewards, values, dones] = get();
    int64_t N = heatmaps.size(0);
    if (N == 0) {
        return std::make_tuple(
            torch::Tensor(), torch::Tensor(), torch::Tensor(),
            torch::Tensor(), torch::Tensor(), torch::Tensor(), torch::Tensor()
        );
    }

    // Créer indices et mélanger
    std::vector<int64_t> indices(N);
    std::iota(indices.begin(), indices.end(), 0);
    std::shuffle(indices.begin(), indices.end(), rng);

    // Prendre les batch_size premiers indices (ou tous si batch_size > N)
    int64_t batch_size_actual = std::min(static_cast<int64_t>(batch_size), N);
    auto selected_indices = torch::tensor(indices).slice(0, 0, batch_size_actual).to(torch::kCUDA);

    // Indexer les tenseurs
    return std::make_tuple(
        heatmaps.index_select(0, selected_indices),
        agent_states.index_select(0, selected_indices),
        actions.index_select(0, selected_indices),
        log_probs.index_select(0, selected_indices),
        rewards.index_select(0, selected_indices),
        values.index_select(0, selected_indices),
        dones.index_select(0, selected_indices)
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