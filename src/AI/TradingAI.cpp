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