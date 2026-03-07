#include "Chat.h"

Chat::Chat(int chat_id) : chat_id_(chat_id){}

void Chat::sendMessage(const Message& message) {
	messages_.push_back(message);
}

const std::vector<Message>& Chat::getMessages() const
{
	return messages_;
}