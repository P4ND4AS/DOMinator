#include "AI/TradingAI.h"

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