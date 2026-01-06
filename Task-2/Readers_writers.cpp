#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
#include <semaphore>

int readCount = 0;
int sharedNum = 1;

std::mutex rdCnt;
std::binary_semaphore wrt(1); 
std::mutex io;

void reader(int id)
{
    rdCnt.lock();
    readCount++;
    if (readCount == 1)
        wrt.acquire();
    rdCnt.unlock();

    io.lock();
    std::cout << "Reader : " << id << " reads sharedNum = " << sharedNum << std::endl;
    io.unlock();

    rdCnt.lock();
    readCount--;
    if (readCount == 0)
        wrt.release();  
    rdCnt.unlock();
}

void writer(int id)
{
    wrt.acquire();       

    sharedNum++;
    io.lock();
    std::cout << "Writer : " << id << " updates sharedNum to : " << sharedNum << std::endl;
    io.unlock();

    wrt.release();      
}

void readers_writers()
{
    const int numReaders = 3;
    const int numWriters = 2;

    std::vector<std::thread> readers;
    std::vector<std::thread> writers;

    for (int i = 0; i < numReaders; i++)
        readers.emplace_back(reader, i + 1);

    for (int i = 0; i < numWriters; i++)
        writers.emplace_back(writer, i + 1);

    for (auto& t : readers)
        t.join();

    for (auto& t : writers)
        t.join();

}
