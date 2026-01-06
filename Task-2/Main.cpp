#include <iostream>

void readers_writers();
void dining_philosophers();
void sleeping_barbers();
void producer_consumer();

int main()
{
    std::cout << "1. Readers-Writers Problem\n";
    std::cout << "2. Dining Philosophers Problem\n";
    std::cout << "3. Sleeping Barbers Problem\n";
    std::cout << "4. Producer Consumer Problem\n";
    std::cout << "Choose: ";

    int choice;
    std::cin >> choice;

    if (choice == 1)
        readers_writers();
    else if (choice == 2)
        dining_philosophers();
    else if (choice == 3)
        sleeping_barbers();
    else if (choice == 4)
        producer_consumer();
    else
        std::cout << "Invalid choice\n";

    return 0;
}
