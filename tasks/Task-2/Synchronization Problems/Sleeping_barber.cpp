#include <iostream>
#include <thread>
#include <vector>
#include <semaphore>
#include <mutex>
#include <chrono>

const int CHAIRS = 3;

std::binary_semaphore barber(0);
std::counting_semaphore<CHAIRS> customers(0);
std::mutex shopMutex;
std::mutex ios;

int waiting = 0;

void barberFunc()
{ 
    while (true)
    {
        customers.acquire();

        shopMutex.lock();
        waiting--;
        ios.lock();
        std::cout << "Barber is cutting hair. Waiting customers = " << waiting << std::endl;
        ios.unlock();
        shopMutex.unlock();

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        barber.release();
    }
}

void customerFunc(int id)
{
    shopMutex.lock();
    if (waiting == CHAIRS)
    {
        ios.lock();
        std::cout << "Customer " << id << " leaves (no chair available)" << std::endl;
        ios.unlock();
        shopMutex.unlock();
        return;
    }

    waiting++;
    ios.lock();
    std::cout << "Customer " << id << " sits in waiting room. Waiting = " << waiting << std::endl;
    ios.unlock();
    shopMutex.unlock();

    customers.release();
    barber.acquire();

    ios.lock();
    std::cout << "Customer " << id << " got a haircut and leaves" << std::endl;
    ios.unlock();
}

void sleeping_barbers()
{
    std::thread barberThread(barberFunc);

    std::vector<std::thread> customerThreads;
    for (int i = 1; i <= 10; i++)
    {
        customerThreads.emplace_back(customerFunc, i);
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    for (auto& t : customerThreads)
        t.join();

    barberThread.detach();
}
