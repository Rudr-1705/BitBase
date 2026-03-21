#include "Chat.h"

Chat::Chat(int chat_id) : chat_id_(chat_id) {}

void Chat::sendMessage(const Message& message)
{
    std::lock_guard<std::mutex> lock(mtx);
    messages_.push_back(message);
}

std::vector<Message> Chat::getMessages() const
{
    std::lock_guard<std::mutex> lock(mtx);
    return messages_;
}