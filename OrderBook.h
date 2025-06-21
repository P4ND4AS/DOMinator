#pragma once
#include "Order.h"

struct BookSnapshot {
    std::map<double, std::vector<LimitOrder>> prices;  // Les ordres à chaque niveau de prix
    double last_price;                            // Dernier prix exécuté
    Side last_side;

    BookSnapshot() : last_price(0.0), last_side(Side::BID) {}
};


class OrderBook {
public:
    OrderBook(double initialPrice = 20000.00, int timestep = 500, int depth = 6, double ticksize = 0.25);

    void initialize_book();
    void print_book_history() const;
    LimitOrder addLimitOrder();
    void setInitialLiquidity(int n_orders);
    MarketOrder generateMarketOrder();
    void processMarketOrder(const MarketOrder& order);
    void modifyLiquidity();
    void update(int n_iter);


private:
    double initialPrice;
    double ticksize;
    int timestep;
    int depth;
    int orderIndex;


    std::chrono::system_clock::time_point currentTime;
    std::chrono::system_clock::time_point initStartTime();
    std::string formatTimestamp(const std::chrono::system_clock::time_point& tp) const;
    std::vector<double> prices;
    BookSnapshot currentBook;

    double lastPrice;
    Side lastSide = Side::BID;

    double currentBestBid = initialPrice - ticksize;
    double currentBestAsk = initialPrice + ticksize;

    std::vector<double> bestBids;
    std::vector<double> bestAsks;

    std::map<std::string, BookSnapshot> bookHistory;
};
