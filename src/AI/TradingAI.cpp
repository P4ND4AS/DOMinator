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
	: orderBook(book), heatmap(128, 128), network(network)
{

}


Action TradingEnvironment::sampleFromPolicy(const Eigen::VectorXf& policy,
	std::mt19937& rng) {
	if (policy.size() != 3) { throw std::invalid_argument("Policy must have 3 elements"); }

	std::discrete_distribution<int> dist(policy.begin(), policy.end());
	int action_index = dist(rng);
	return static_cast<Action>(action_index);
}

void TradingEnvironment::handleAction(Action action) {
    int current_timestep = current_decision_index;  // ou autre variable qui compte les itérations

    if (action != WAIT || agent_state.position != 0) {
        RewardWindow rw(current_decision_index, action, agent_state.entry_price,
            agent_state.position != 0);
        reward_windows.push_back(rw);
    }

    if (action == BUY_MARKET) {
        if (agent_state.position == 0) {
            // Ouverture d'une nouvelle position longue
            agent_state.position = 1;
            agent_state.entry_price = orderBook->getCurrentBestAsk();

            // Nouveau trade ouvert
            open_trade = TradeLog{
                current_timestep,
                agent_state.entry_price,
                -1,  // pas encore fermé
                0.0f,
                0.0f,
                BUY_MARKET,
                WAIT  // pas encore de sortie
            };
        }
        else if (agent_state.position == -1) {
            // Fermeture d'une position courte
            float exit_price = orderBook->getCurrentBestAsk();
            float pnl = agent_state.entry_price - exit_price;

            // Compléter le trade ouvert
            if (open_trade.has_value()) {
                open_trade->timestep_exit = current_timestep;
                open_trade->price_exit = exit_price;
                open_trade->pnl = pnl;
                open_trade->exit_action = BUY_MARKET;
                trade_logs.push_back(open_trade.value());
                open_trade.reset();
            }

            agent_state.position = 0;
        }
    }
    else if (action == SELL_MARKET) {
        if (agent_state.position == 0) {
            // Ouverture d'une nouvelle position courte
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
        }
        else if (agent_state.position == 1) {
            // Fermeture d'une position longue
            float exit_price = orderBook->getCurrentBestBid();
            float pnl = exit_price - agent_state.entry_price;

            if (open_trade.has_value()) {
                open_trade->timestep_exit = current_timestep;
                open_trade->price_exit = exit_price;
                open_trade->pnl = pnl;
                open_trade->exit_action = SELL_MARKET;
                trade_logs.push_back(open_trade.value());
                open_trade.reset();
            }

            agent_state.position = 0;
        }
    }
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
    for (int traj = 0; traj < N_trajectories; ++traj) {
        for (int iter = 0; iter < traj_duration * decision_per_second; ++iter) {
            orderBook->update(marketUpdatePerDecision, rng);
            heatmap.updateData(orderBook->getCurrentBook());
            auto [policy, value] = network->forward(heatmap.data, agent_state.toVector());
            Action action = sampleFromPolicy(policy, rng);
            handleAction(action);
            updateRewardWindows();

        }
        printTradeLogs();
    }
}