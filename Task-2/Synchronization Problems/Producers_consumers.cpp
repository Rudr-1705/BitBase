#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

using namespace std;

queue<int> buffer;
const int BUFFER_SIZE = 5;

mutex mtx;
condition_variable cv_not_full, cv_not_empty;

void producer()
{
    int item = 0;

    for(int i = 1; i <= 10; i++)
    {
        item++;

        unique_lock<mutex> lock(mtx);

        while (buffer.size() == BUFFER_SIZE)
            cv_not_full.wait(lock);

        buffer.push(item);
        cout << "Produced: " << item << endl;

        cv_not_empty.notify_one();
    }
}

void consumer()
{
    for(int i = 1; i <= 10; i++)
    {
        unique_lock<mutex> lock(mtx);

        while (buffer.empty())
            cv_not_empty.wait(lock);

        int item = buffer.front();
        buffer.pop();
        cout << "Consumed: " << item << endl;

        cv_not_full.notify_one();
    }
}

void producer_consumer()
{
    thread p(producer);
    thread c(consumer);

    p.join();
    c.join();
}
