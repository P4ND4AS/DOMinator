#pragma once
#include "Order.h"
#include "densities/density_brownian.h"
#include <random>

const extern double initialPrice;
const extern double ticksize;
const extern int timestep;
const extern int depth;
const extern Side lastSide;
const extern double PI;

struct BookSnapshot {
    std::map<double, std::vector<LimitOrder>> prices;  // Les ordres à chaque niveau de prix
    double last_price;                            // Dernier prix exécuté
    Side last_side;

    BookSnapshot() : last_price(20000.00), last_side(Side::ASK) {}
};


class OrderBook {
public:
    OrderBook();

    void initialize_book();
    void print_book_history() const;
    const BookSnapshot& getCurrentBook() const { return currentBook; }
    const double& getCurrentLastPrice() const { return currentBook.last_price; }
    LimitOrder addLimitOrder(std::mt19937& rng);
    void setInitialLiquidity(int n_orders, std::mt19937& rng);
    MarketOrder generateMarketOrder();
    void processMarketOrder(const MarketOrder& order);
    void cancelLiquidity(std::mt19937& rng);
    void update(int n_iter, std::mt19937& rng);

    std::vector<Foyer> getCurrentFoyersState() const { return foyers_states; }

    double getCurrentBestAsk() const { return currentBestAsk; }
    double getCurrentBestBid() const { return currentBestBid; }

private:
    int orderIndex;

    int64_t currentTime;
    std::chrono::system_clock::time_point initStartTime();
    std::string formatTimestamp(const std::chrono::system_clock::time_point& tp) const;
    std::vector<double> prices;
    BookSnapshot currentBook;

    double currentBestBid = initialPrice - ticksize;
    double currentBestAsk = initialPrice + ticksize;

    double minPrice = initialPrice - depth * ticksize;
    double maxPrice = initialPrice + depth * ticksize;

    std::vector<double> bestBids;
    std::vector<double> bestAsks;

    std::map<int64_t, BookSnapshot> bookHistory;

    std::vector<Foyer> foyers_states = { {gSimuParams.addLiq.priceDist.mu_init, 
                                          gSimuParams.addLiq.priceDist.sigma_init } };
};
