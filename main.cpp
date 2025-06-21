#include "OrderBook.h"
#include <iostream>
#include <string> 
#include <windows.h>
#include <chrono>

int main() {
    //SetConsoleOutputCP(CP_UTF8);
    std::cout << "Cr�ation OrderBook..." << std::endl;
    OrderBook ob;
    std::cout << "Ajout liquidit� initiale..." << std::endl;
    ob.setInitialLiquidity(15);

    const int n_iter = 10;
    std::cout << "D�but simulation..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();

    ob.update(n_iter);

    ob.print_book_history();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration_micro = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    double duration_sec = duration_micro / 1e6;

    std::cout << "\nSimulation termin�e en " << duration_sec << " secondes (" << duration_micro << " �s)\n";

    system("pause");
    return 0;
}