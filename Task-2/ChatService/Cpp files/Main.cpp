#include "ChatService.h"
#include <iostream>
#include <cassert>

void quickConcurrency(ChatService& service, int chatId, int userId)
{
    std::thread t1([&] {
        for (int i = 0; i < 4; i++)
            service.sendMessage(chatId, userId, "A");
        });

    std::thread t2([&] {
        for (int i = 0; i < 4; i++)
            service.sendMessage(chatId, userId, "B");
        });

    t1.join();
    t2.join();
}

void printMessages(const ChatService& service, int chatId)
{
    auto messages = service.getMessages(chatId);

    if (messages.empty())
    {
        std::cout << "No messages (or invalid chat)\n\n";
        return;
    }

    std::cout << "Messages in chat " << chatId << ":\n";
    for (const auto& msg : messages)
    {
        std::cout << "  User " << msg.getSenderId()
            << ": " << msg.getText() << '\n';
    }
    std::cout << std::endl;
}

int main()
{
    ChatService service;

    std::cout << "===== USER CREATION =====\n";
    auto alice = service.createUser("Alice");
    auto bob = service.createUser("Bob");
    auto carl = service.createUser("Carl");

    assert(alice && bob && carl);

    std::cout << "Users created:\n";
    std::cout << "  Alice ID: " << *alice << '\n';
    std::cout << "  Bob   ID: " << *bob << '\n';
    std::cout << "  Carl  ID: " << *carl << "\n\n";

    std::cout << "===== DIRECT CHAT TEST =====\n";
    auto directChat = service.createDirectChat(*alice, *bob);
    assert(directChat);

    service.sendMessage(*directChat, *alice, "Hey Bob!");
    service.sendMessage(*directChat, *bob, "Hey Alice!");
    service.sendMessage(*directChat, *alice, "How are you?");

    printMessages(service, *directChat);

    assert(service.getMessages(*directChat).size() == 3);

    std::cout << "===== GROUP CHAT TEST =====\n";
    auto groupChat = service.createGroupChat({ *alice, *bob, *carl });
    assert(groupChat);

    service.sendMessage(*groupChat, *alice, "Hello everyone!");
    service.sendMessage(*groupChat, *bob, "Hey!");
    service.sendMessage(*groupChat, *carl, "What’s up?");

    printMessages(service, *groupChat);

    assert(service.getMessages(*groupChat).size() == 3);

    std::cout << "===== FAILURE / EDGE CASES =====\n";

    std::cout << "\n-- Invalid user --\n";
    service.sendMessage(*directChat, 9999, "This should fail");

    std::cout << "\n-- Invalid chat --\n";
    service.sendMessage(9999, *alice, "This should fail");

    std::cout << "\n-- User not in chat --\n";
    service.sendMessage(*directChat, *carl, "I should not be here");

    std::cout << "\n-- Direct chat with same user --\n";
    auto badDirect = service.createDirectChat(*alice, *alice);
    assert(!badDirect);

    std::cout << "\n-- Empty group chat --\n";
    auto badGroup = service.createGroupChat({});
    assert(!badGroup);

    std::cout << "\n===== QUICK CONCURRENCY TESTS =====\n";

    auto c = service.createDirectChat(*alice, *bob);
    assert(c);

    quickConcurrency(service, *c, *alice);

    auto msgs = service.getMessages(*c);
    assert(msgs.size() == 8);

    std::cout << "Quick concurrency test passed\n";


    std::cout << "\n===== ALL TESTS PASSED =====\n";

    std::cin.get();
    return 0;
}