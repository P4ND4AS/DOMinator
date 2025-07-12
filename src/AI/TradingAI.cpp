#include "AI/TradingAI.h"
#include "Heatmap.h"


void MemoryBuffer::store(const Experience& exp) {
	buffer.push_back(exp);
}

void MemoryBuffer::clear() {
	buffer.clear();
}

const std::vector<Experience>& MemoryBuffer::get() const {
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


TradingEnvironment::TradingEnvironment(OrderBook* book)
	: orderBook(book), heatmap(128, 128), timestep(0), maxTimesteps(5000)
{
	reset();
}

void TradingEnvironment::reset() {
	timestep = 0;
	episodeDone = false;
	cumulativeReward = 0.0f;
	currentPosition = 0;
	entryPrice = 0.0;
}

void TradingEnvironment::updateMarket(int n_iter, std::mt19937& rng) {
	orderBook->update(n_iter, rng);
	timestep += n_iter;
	if (timestep >= maxTimesteps) {
		episodeDone = true;
	}
}

std::vector<std::vector<float>> TradingEnvironment::getObservation() {
	BookSnapshot snapshot = orderBook->getCurrentBook();

	heatmap.updateData(snapshot);

	return heatmap.data;
}


Eigen::VectorXf TradingEnvironment::getAgentState() const {
	Eigen::VectorXf state(1);
	state(0) = static_cast<float>(currentPosition);
	return state;
}


float TradingEnvironment::step(Action action, std::mt19937& rng) {
	float reward = 0.0f;
	const double bestBid = orderBook->getCurrentBestBid();
	const double bestAsk = orderBook->getCurrentBestAsk();

	switch (action) {
	case BUY_MARKET:
		if (currentPosition == 0) {
			MarketOrder buyOrder = orderBook->generateMarketOrder();
			buyOrder.side = Side::ASK;
			buyOrder.size = 1;
			orderBook->processMarketOrder(buyOrder);

			entryPrice = bestAsk;
			currentPosition = 1;
		}
		else if (currentPosition == -1) {
			MarketOrder buyCover = orderBook->generateMarketOrder();
			buyCover.side = Side::ASK;
			buyCover.size = 1;
			orderBook->processMarketOrder(buyCover);

			reward = static_cast<float>(entryPrice - bestAsk);
			currentPosition = 0;
			entryPrice = 0.0;
		}
		break;

	case SELL_MARKET:
		if (currentPosition == 0) {
			MarketOrder sellOrder = orderBook->generateMarketOrder();
			sellOrder.side = Side::BID;
			sellOrder.size = 1;
			orderBook->processMarketOrder(sellOrder);

			entryPrice = bestBid;
			currentPosition = -1;
		}
		else if (currentPosition == 1) {
			MarketOrder sellClose = orderBook->generateMarketOrder();
			sellClose.side = Side::BID;
			sellClose.size = 1;
			orderBook->processMarketOrder(sellClose);

			reward = static_cast<float>(bestBid - entryPrice);
			currentPosition = 0;
			entryPrice = 0.0;
		}
		break;

	case WAIT:
		break;
	}

	updateMarket(20000, rng); 

	if (currentPosition == 1) {
		reward = static_cast<float>(bestBid - entryPrice);
	}
	else if (currentPosition == -1) {
		reward = static_cast<float>(entryPrice - bestAsk);
	}
	cumulativeReward += reward;
	return reward;
}