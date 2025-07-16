#include "AI/TradingAI.h"
#include "Heatmap.h"
#include <iostream>

/*void MemoryBuffer::store(const Transition& exp) {
	buffer.push_back(exp);
}

void MemoryBuffer::clear() {
	buffer.clear();
}

const std::vector<Transition>& MemoryBuffer::get() const {
	return buffer;
}

std::vector<float> MemoryBuffer::computeAdvantages(float lastValue) {
	std::vector<float> advantages(buffer.size());
	std::vector<float> deltas(buffer.size());

	float nextValue = lastValue;
	for (int t = buffer.size() - 1; t >= 0; --t) {
		float reward = buffer[t].reward;
		float value = buffer[t].value;
		bool done = buffer[t].done;

		float delta = reward + (done ? 0.0f : GAMMA * nextValue) - value;
		deltas[t] = delta;
		nextValue = value;
	}

	float adv = 0.0f;
	for (int t = buffer.size() - 1; t >= 0; --t) {
		adv = deltas[t] + GAMMA * LAMBDA * adv;
		advantages[t] = adv;
	}

    // Après le calcul des advantages :
    float mean = std::accumulate(advantages.begin(), advantages.end(), 0.0f) / advantages.size();
    float var = 0.0f;
    for (float a : advantages) var += (a - mean) * (a - mean);
    var /= advantages.size();
    float stddev = std::sqrt(var + 1e-8f);

    for (float& a : advantages) {
        a = (a - mean) / stddev;
    }


	return advantages;
}


std::vector<float> MemoryBuffer::computeReturns(float lastValue) {
	std::vector<float> returns(buffer.size());

	float ret = lastValue;
	for (int t = buffer.size() - 1; t >= 0; --t) {
		float reward = buffer[t].reward;
		bool done = buffer[t].done;
		ret = reward + (done ? 0.0f : GAMMA * ret);
		returns[t] = ret;
	}
	return returns;
}

// --------------------- TRADING ENVIRONMENT FOR TRAINING ---------------------


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