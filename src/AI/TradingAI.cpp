#include "AI/TradingAI.h"
#include "Heatmap.h"
#include <iostream>

void MemoryBuffer::store(const Transition& exp) {
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
	: orderBook(book), heatmap(128, 30), network(network)
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

    if (action == BUY_MARKET) {
        std::cout << "BUY" << "\n";
        if (agent_state.position == 0) {
            std::cout<<"long opened"<<"\n";
            agent_state.position = 1;
            agent_state.entry_price = orderBook->getCurrentBestAsk();

            open_trade = TradeLog{
                current_timestep,
                agent_state.entry_price,
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
            float exit_price = orderBook->getCurrentBestAsk();
            float pnl = agent_state.entry_price - exit_price;

            if (open_trade.has_value()) {
                std::cout << "long closed" << "\n";
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
        }
    }
    else if (action == SELL_MARKET) {
        if (agent_state.position == 0) {
            agent_state.position = -1;
            agent_state.entry_price = orderBook->getCurrentBestBid();

            open_trade = TradeLog{
                current_timestep,
                agent_state.entry_price,
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
            float exit_price = orderBook->getCurrentBestBid();
            float pnl = exit_price - agent_state.entry_price;

            if (open_trade.has_value()) {
                std::cout << "short closed" << "\n";
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
        }
    }

    RewardWindow rw(current_decision_index, action, agent_state.entry_price,
        agent_state.position != 0, policy(static_cast<int>(action)), value);
    reward_windows.push_back(rw);
}


void TradingEnvironment::updateRewardWindows() {

	for (auto it = reward_windows.begin(); it != reward_windows.end();) {
		it->addPnL(orderBook->getCurrentBestBid(), orderBook->getCurrentBestAsk(), agent_state);

		if (it->isComplete()) {
			float reward = it->computeWeightedReward();

			Transition transition;
			transition.action = it->action;
			transition.log_prob = std::log(it->proba);
			transition.reward = reward;
			transition.value = it->value;
			transition.done = isEpisodeDone;

			buffer.store(transition);

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
            << " | EntryAction=" << static_cast<int>(trade.entry_action)
            << " ExitAction=" << static_cast<int>(trade.exit_action)
            << "\n";
    }
}


void TradingEnvironment::train(std::mt19937& rng) {
    Eigen::MatrixXf dummy_heatmap = Eigen::MatrixXf::Zero(401, 128);
    for (int traj = 0; traj < N_trajectories; ++traj) {
        std::cout << "Trajectory #" << traj << "\n\n";
        
        for (int iter = 0; iter < 128; ++iter) {
            orderBook->update(200, rng);
            heatmap.updateData(orderBook->getCurrentBook());
        }
        std::cout << "OrderBook sped up a few seconds..." << "\n";

        for (int iter = 0; iter < traj_duration * decision_per_second; ++iter) {
            std::cout << "Iteration #" << iter << "\n\n";
            orderBook->update(marketUpdatePerDecision, rng);
            std::cout << "BestBid : " << orderBook->getCurrentBestBid() << " & BestAsk : " << orderBook->getCurrentBestAsk() << "\n";
            heatmap.updateData(orderBook->getCurrentBook());
            std::cout << "Matrice updated" << "\n";
            auto [policy, value] = network->forward(dummy_heatmap, agent_state.toVector());

            std::cout << "Policy : " << policy << " & value : " << value << "\n";

            Action action = sampleFromPolicy(policy, rng);
            std::cout << "Action : " << action << "\n";
            handleAction(action, policy, value);
            std::cout << "Action handled" << "\n";
            updateRewardWindows();
            std::cout << "Reward windows updated" << "\n";
            std::cout << "\n\n";

        }
        printTradeLogs();
    }
}