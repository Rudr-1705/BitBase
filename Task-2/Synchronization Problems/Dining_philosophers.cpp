#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <chrono>

const int N = 5;


std::mutex iod;
std::mutex forkMutex[N];

void philosopher(int id)
{
    int left = id;
    int right = (id + 1) % N;

    if (id % 2 == 1) {
        forkMutex[left].lock();
        forkMutex[right].lock();
    }
    else {
        forkMutex[right].lock();
        forkMutex[left].lock();
    }

    // simulating eating
    iod.lock();
    std::cout << "Philosopher " << id << " is using fork " << left << " & " << right << std::endl;
    iod.unlock();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    forkMutex[left].unlock();
    forkMutex[right].unlock();

}

void dining_philosophers()
{
    std::vector<std::thread> philosophers;

    for (int i = 0; i < N; i++)
        philosophers.emplace_back(philosopher, i);

    for (auto& t : philosophers)
        t.join();
}
