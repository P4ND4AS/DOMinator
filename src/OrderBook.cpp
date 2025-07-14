#include "engine/OrderBook.h"
#include "utils.h"
#include "densities/densities_add_liq.h"
#include "densities/densities_mod_liq.h"
#include <algorithm>
#include <cstdint>
#include <vector>
#include <cmath>

#include <iostream>
#include <random>
#include <ctime>
#include <sstream>
#include <iomanip>


const double initialPrice = 20000.00;
const double ticksize = 0.25;
const int timestep = 5;
const int depth = 200;
const Side lastSide = Side::BID;
const double PI = 3.141592653589793;



std::chrono::system_clock::time_point OrderBook::initStartTime() {
    std::tm tm = {};
    tm.tm_year = 2025 - 1900;
    tm.tm_mon = 5;      // Juin = mois 5
    tm.tm_mday = 18;
    tm.tm_hour = 14;
    tm.tm_min = 30;
    tm.tm_sec = 0;

    std::time_t time_start = std::mktime(&tm);
    return std::chrono::system_clock::from_time_t(time_start);
}

std::string OrderBook::formatTimestamp(const std::chrono::system_clock::time_point& tp) const {     // Fonction très coûteuse en temps d'exécution
    auto duration = tp.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration - seconds);

    std::time_t tt = std::chrono::system_clock::to_time_t(tp);
    std::tm tm;
    localtime_s(&tm, &tt);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
        << "." << std::setfill('0') << std::setw(6) << microseconds.count();

    return oss.str();
}


OrderBook::OrderBook()
    :orderIndex(0)
{
    // Initialisation du vecteur des prix autour du prix initial
    for (int i = -depth; i <= depth; ++i) {
        prices.push_back(initialPrice + i * ticksize);
    }

    // Initialiser l'heure actuelle comme point de départ (à affiner)
    currentTime = 0;

    //Initialiser le carnet
    initialize_book();
}


void OrderBook::initialize_book() {

    for (double price : prices) {
        currentBook.prices[price] = {};
    }
    currentBook.last_price = initialPrice;
    currentBook.last_side = lastSide;
    
    // bookHistory[currentTime] = currentBook;

}

void OrderBook::setInitialLiquidity(int n_orders, std::mt19937& rng) {
    for (int i = 0; i < n_orders; ++i) {
        try {
            LimitOrder order = addLimitOrder(rng);
            currentBook.prices[order.price].push_back(order);
        }
        catch (const std::exception& e) {
            std::cerr << "Erreur lors de la génération d'un ordre limite: " << e.what() << std::endl;
            break;
        }
    }
    //bookHistory[currentTime] = currentBook;
    //std::cout << "CurrentBestBid: " << currentBestBid << "\nand CurrentBestAsk: " << currentBestAsk << '\n';
}


void OrderBook::print_book_history() const {
    for (const auto& entry : bookHistory) {
        auto timestamp = entry.first;
        auto snapshot = entry.second;
        std::cout << "Timestamp: " << timestamp << "\n";
        std::cout << "Last Price: " << std::fixed << std::setprecision(2) << snapshot.last_price << "\n";
        std::cout << "Last Side: " << sideToString(snapshot.last_side) << "\n";

        for (const auto& level : snapshot.prices) {
            auto price = level.first;
            auto orders = level.second;
            std::cout << std::fixed << std::setprecision(2);
            std::cout << "Price: " << price << " | Orders : [";

            if (orders.empty()) {
                std::cout << "]\n";
                continue;
            }

            std::cout << "\n";
            for (const auto& order : orders) {
                std::cout << "    ID: " << order.id
                    << ", Size: " << order.size
                    << ", Side: " << sideToString(order.side)
                    << ", Time: " << order.timestamp << "\n";
            }
            std::cout << "]\n";
        }

        std::cout << "--------------------------\n";
    }
}




LimitOrder OrderBook::addLimitOrder(std::mt19937& rng) {

    double S = currentBestAsk - currentBestBid;
    double q1Bid = 0;// getVolumeAt(...);
    double q1Ask = 0;// getVolumeAt(...);

    Side side = sampleAddLiqSide(gSimuParams, S, q1Ask, q1Bid, rng);

    int size = sampleAddLiqSize(gSimuParams, rng);
 
    double price = sampleAddLiqPrice(gSimuParams, side, currentBestBid, currentBestAsk,
        minPrice, maxPrice, prices, foyers_states, rng);


    if (side == Side::BID && price > currentBestBid) {
        currentBestBid = price;
    }
    if(side == Side::ASK && price < currentBestAsk) {
        currentBestAsk = price;
    }

    LimitOrder order;
    order.id = orderIndex++;
    order.price = price;
    order.size = size;
    order.side = side;
    order.timestamp = currentTime;

    return order;
}



void OrderBook::cancelLiquidity(std::mt19937& rng) {
    if (currentBook.prices.empty()) return;


    double price = sampleRemoveLiqPrice(gSimuParams, currentBestBid, currentBestAsk, minPrice,
                                        maxPrice, prices, foyers_states, rng);

    auto& listOfOrders = currentBook.prices[price];

    if (!listOfOrders.empty()) {
        
        std::uniform_int_distribution<> order_dist(0, static_cast<int>(listOfOrders.size())- 1);
        int randomIndex = order_dist(rng);
        listOfOrders.erase(listOfOrders.begin() + randomIndex);

        if (price == currentBestAsk && listOfOrders.size() == 0) {
            auto remainingOrders = currentBook.prices[price];
            while (remainingOrders.empty()) {
                price += ticksize;
                remainingOrders = currentBook.prices[price];
            }
            currentBestAsk = price;
        }

        if (price == currentBestBid && listOfOrders.size() == 0) {
            auto remainingOrders = currentBook.prices[price];
            while (remainingOrders.empty()) {
                price -= ticksize;
                remainingOrders = currentBook.prices[price];
            }
            currentBestBid = price;
        }
    }
}



MarketOrder OrderBook::generateMarketOrder() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<int> side_dist(0, 1);
    static std::exponential_distribution<> size_dist(1.0);

    MarketOrder order;
    order.side = (side_dist(gen) == 0) ? Side::ASK : Side::BID;
    order.size = static_cast<int>(size_dist(gen)) + 1;
    order.timestamp = currentTime;
    return order;
}


void OrderBook::processMarketOrder(const MarketOrder& order) {
    int size = order.size;
    Side side = order.side;

    int priceMultiplier = (side == Side::ASK) ? 1 : -1;
    double startPrice = (side == Side::ASK ) ? currentBestAsk : currentBestBid;
    //std::cout << "StartPrice: " << startPrice << "\n";

    int sumVolumes = 0;

    while (sumVolumes < size) {
        auto listOfOrders = currentBook.prices[startPrice];
        
        while (listOfOrders.empty()) {
            //std::cout << "Pas de liquidité à: " << startPrice << "\n";
            startPrice += priceMultiplier * ticksize;
            if (!isPriceInRange(startPrice, minPrice, maxPrice)) {
                std::cerr << "[ERREUR] Le prix " << startPrice
                    << " sort de la gamme autorisée [" << minPrice << " ; " << maxPrice << "].\n";
                exit(1); 
            }
            listOfOrders = currentBook.prices[startPrice];
        }

        auto limitOrder = listOfOrders.front();
        int limitOrderSize = limitOrder.size;

        sumVolumes += limitOrderSize;

        if (sumVolumes <= size) {
            listOfOrders.erase(listOfOrders.begin());
            currentBook.prices[startPrice] = listOfOrders;
            if (sumVolumes == size) {
                //std::cout << "Size=sumVolumes" << "\n";
                currentBook.last_price = startPrice;
                currentBook.last_side = side;
                // Ici, comme il se peut qu'il n'y ait de liquidité sur 'startPrice', on doit
                // trouver la liquidité la plus proche.
                auto remainingOrders = currentBook.prices[startPrice]; // Pas de '&' car on ne veut pas modifier
                                                                       // le 'currentBook'.
            
                while (remainingOrders.empty()) {
                    //std::cout << "Pas d'ordre à :" << startPrice << "\n";
                    startPrice += priceMultiplier * ticksize;
                    remainingOrders = currentBook.prices[startPrice];
                }
                if (side == Side::ASK) {
                    currentBestAsk = startPrice;
                }
                else {
                    currentBestBid = startPrice;
                }
                break;
            }
        }
        else {
            currentBook.last_price = startPrice;
            currentBook.last_side = side;
            currentBook.prices[startPrice].front().size -= (limitOrderSize + size - sumVolumes);
            if (side == Side::ASK) {
                currentBestAsk = startPrice;
            }
            else {
                currentBestBid = startPrice;
            }
            break;
        }
    }
}


void OrderBook::update(int n_iter, std::mt19937& rng) {
    double& p_death = gSimuParams.addLiq.priceDist.p_death;
    double& p_birth = gSimuParams.addLiq.priceDist.p_birth;
    double& sigma_init = gSimuParams.addLiq.priceDist.sigma_init;
    double& mu_jitter = gSimuParams.addLiq.priceDist.mu_jitter;
    double& sigma_jitter = gSimuParams.addLiq.priceDist.sigma_jitter;

    for (int i = 0; i < n_iter; ++i) {

        currentTime += timestep;
        updateFoyerState(foyers_states, prices, rng, p_death, p_birth, sigma_init, mu_jitter, sigma_jitter);

        double p_add_liq = sampleLambdaL(gSimuParams, 0, 0, 0, rng);

        std::discrete_distribution<> event_dist({ p_add_liq, 0.006, 0.0004, 0.9886 });
        int eventType = event_dist(rng);

        if (eventType == 0) {
            // ADD LIMIT ORDER
            LimitOrder limitOrder = addLimitOrder(rng);
            currentBook.prices[limitOrder.price].push_back(limitOrder);
            //std::cout << "Ordre limite ajout à: " << limitOrder.price << ", size: " << limitOrder.size << " au " << sideToString(limitOrder.side) << "\n";
        }
        else if (eventType == 1) {
            cancelLiquidity(rng);
        }
        else if (eventType == 2) {
            MarketOrder marketOrder = generateMarketOrder();
            //std::cout << "Market order generated: side=" << sideToString(marketOrder.side) << ", size=" << marketOrder.size << ", timestamp=" << marketOrder.timestamp << "\n";
            processMarketOrder(marketOrder);

        }

        //bookHistory[currentTime] = currentBook;
        //bestAsks.push_back(currentBestAsk);
        //bestBids.push_back(currentBestBid);
    }
}
