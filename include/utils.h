#pragma once
#include "Order.h"

inline const char* sideToString(Side side) {
    switch (side) {
    case Side::BID: return "BID";
    case Side::ASK: return "ASK";
    default: return "UNKNOWN";
    }
}


bool isPriceInRange(double price, double minPrice, double maxPrice) {
    return price >= minPrice && price <= maxPrice;
}