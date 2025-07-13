#include <iostream>
#include <thread>
#include <chrono>

int main() {
    std::cout << "Test simple du système de supervision" << std::endl;
    std::cout << "Vérification du support multi-threading..." << std::endl;
    
    // Test simple de threading
    auto start = std::chrono::high_resolution_clock::now();
    
    std::thread t1([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "Thread 1 terminé" << std::endl;
    });
    
    std::thread t2([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "Thread 2 terminé" << std::endl;
    });
    
    t1.join();
    t2.join();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Test terminé en " << duration.count() << "ms" << std::endl;
    std::cout << "Support multi-threading: OK" << std::endl;
    
    return 0;
}

