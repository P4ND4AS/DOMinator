#include "OrderBook.h"
#include "utils.h"
#include <vector>
#include <cmath>
#include <iostream>
#include <random>
#include <ctime>
#include <sstream>
#include <iomanip>

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

std::string OrderBook::formatTimestamp(const std::chrono::system_clock::time_point& tp) const {
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


OrderBook::OrderBook(double initialPrice, int timestep, int depth, double ticksize)
    : initialPrice(initialPrice),
    timestep(timestep),
    depth(depth),
    ticksize(ticksize),
    orderIndex(0),
    lastPrice(lastPrice),
    lastSide(lastSide)
{
    // Initialisation du vecteur des prix autour du prix initial
    for (int i = -depth; i <= depth; ++i) {
        prices.push_back(initialPrice + i * ticksize);
    }

    // Initialiser l'heure actuelle comme point de départ (à affiner)
    currentTime = initStartTime();

    //Initialiser le carnet
    initialize_book();
}


void OrderBook::initialize_book() {

    for (double price : prices) {
        currentBook.prices[price] = {};
    }
    currentBook.last_price = initialPrice;
    currentBook.last_side = lastSide;

    std::string timestamp_str = formatTimestamp(currentTime);
    bookHistory[timestamp_str] = currentBook;

}

void OrderBook::setInitialLiquidity(int n_orders) {
    for (int i = 0; i < n_orders; ++i) {
        try {
            LimitOrder order = addLimitOrder();
            currentBook.prices[order.price].push_back(order);
            if ((order.side == Side::BID) && order.price > currentBestBid) {
                currentBestBid = order.price;
            }
            if ((order.side == Side::ASK) && order.price < currentBestAsk) {
                currentBestAsk = order.price;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Erreur lors de la génération d'un ordre limite: " << e.what() << std::endl;
            break;
        }
    }
    std::string timestamp_str = formatTimestamp(currentTime);
    bookHistory[timestamp_str] = currentBook;
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




LimitOrder OrderBook::addLimitOrder() {

    static std::random_device rd;
    static std::mt19937 gen(rd());

    std::uniform_int_distribution<> side_dist(0, 1);
    std::uniform_int_distribution<> size_dist(1, 5);
    Side side = (side_dist(gen) == 0) ? Side::BID : Side::ASK;
    int size = size_dist(gen);

    double price;


    // Calcul les niveaux de prix valides
    std::vector<double> valid_prices;
    for (double p : prices) {
        if ((side == Side::BID && p < currentBestAsk) ||
            (side == Side::ASK && p > currentBestBid)) {
            valid_prices.push_back(p);
        }
    }

    if (valid_prices.empty()) {
        throw std::runtime_error("No valid price levels available for limit order");
    }

    std::uniform_int_distribution<> price_index_dist(0, static_cast<int>(valid_prices.size()) - 1);;
    price = valid_prices[price_index_dist(gen)];


    LimitOrder order;
    order.id = orderIndex;
    order.price = price;
    order.size = size;
    order.side = side;
    order.timestamp = formatTimestamp(currentTime);

    orderIndex++;
    return order;
}





void OrderBook::modifyLiquidity() {
    //std::cout << "Modify liquidity...\n";
    if (currentBook.prices.empty()) return;

    std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> price_dist(0, static_cast<int>(prices.size()) - 1);
    double randomPrice = prices[price_dist(gen)];

    auto& listOfOrders = currentBook.prices[randomPrice];
    //std::cout << "Modifying orders at price: " << randomPrice << "\n";
    if (!listOfOrders.empty()) {
        // Sélection d'un ordre au hasard dans la liste
        std::uniform_int_distribution<> order_dist(0, static_cast<int>(listOfOrders.size())- 1);
        int randomIndex = order_dist(gen);
        auto& randomOrder = listOfOrders[randomIndex];

        // Décision aléatoire : cancel, volume_change, hold
        std::discrete_distribution<> decision_dist({ 0.5, 0.35, 0.15 });
        int decision = decision_dist(gen);

        switch (decision) {
        case 0: { // CANCEL
            //std::cout << "Cancelling order ID: " << randomOrder.id << "\n";
            listOfOrders.erase(listOfOrders.begin() + randomIndex);
            break;
        }

        case 1: { // VOLUME CHANGE
            std::geometric_distribution<> volume_dist(0.3);
            int delta = volume_dist(gen) + 1;
            double decrease_prob = 0.8;
            std::bernoulli_distribution flip(decrease_prob);

            if (flip(gen)) {
                randomOrder.size -= delta;
                //std::cout << "Decreased size of order ID: " << randomOrder.id << " to " << randomOrder.size << "\n";
                if (randomOrder.size <= 0) {
                    //std::cout << "Order size <= 0. Removing order\n";
                    listOfOrders.erase(listOfOrders.begin() + randomIndex);
                }
            }
            else {
                randomOrder.size += delta;
                //std::cout << "Increased size of order ID: "<<randomOrder.id<<" to "<<randomOrder.size<<"\n";
            }
            break;
        }

        case 2: {
            //std::cout << "Holding order ID: "<<randomOrder.id<<" (unchanged)\n";
            break;
        }
        }
    }
    //std::cout<<">>> modifyLiquidity() completed.\n";
}


MarketOrder OrderBook::generateMarketOrder() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<int> side_dist(0, 1);
    static std::exponential_distribution<> size_dist(1.0);

    MarketOrder order;
    order.side = (side_dist(gen) == 0) ? Side::ASK : Side::BID;
    order.size = static_cast<int>(size_dist(gen)) + 1;
    order.timestamp = formatTimestamp(currentTime);
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
        int j = 0;
        while (listOfOrders.empty() && j < 100) {
            //std::cout << "Pas de liquidité à: " << startPrice << "\n";
            startPrice += priceMultiplier * ticksize;
            listOfOrders = currentBook.prices[startPrice];
            j++;
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
                int max_iter = 0;
                while (remainingOrders.empty() && max_iter < 100) {
                    //std::cout << "Pas d'ordre à :" << startPrice << "\n";
                    startPrice += priceMultiplier * ticksize;
                    remainingOrders = currentBook.prices[startPrice];
                    max_iter++;
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


void OrderBook::update(int n_iter) {

    for (int i = 0; i < n_iter; ++i) {
        //std::cout << "\nItération n°"<< i << "\n";

        currentTime += std::chrono::microseconds(timestep);
        std::string timestamp_str = formatTimestamp(currentTime);

        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::discrete_distribution<> event_dist({ 0.43, 0.55, 0.02 });
        int eventType = event_dist(gen);

        if (eventType == 0) {
            // ADD LIMIT ORDER
            LimitOrder limitOrder = addLimitOrder();
            currentBook.prices[limitOrder.price].push_back(limitOrder);
            //std::cout << "Ordre limite ajouté" << "\n";
        }
        else if (eventType == 1) {
            modifyLiquidity();
        }
        else {
            MarketOrder marketOrder = generateMarketOrder();
            //std::cout << "Market order generated: side=" << marketOrder.side << ", size=" << marketOrder.size << ", timestamp=" << marketOrder.timestamp << "\n";
            processMarketOrder(marketOrder);

        }

        //bookHistory[timestamp_str] = currentBook;
        bestAsks.push_back(currentBestAsk);
        bestBids.push_back(currentBestBid);

        //std::cout << "BestBid: " << bestBids.back() << "\nand BestAsk: " << bestAsks.back() << '\n';

        //std::cout << "\n";
    }
}

