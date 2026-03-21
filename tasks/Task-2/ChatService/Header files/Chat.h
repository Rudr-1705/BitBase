#pragma once
#include <vector>
#include "Message.h"
#include <mutex>

class Chat {
private:
	int chat_id_;
	std::vector<Message> messages_;

protected:
	mutable std::mutex mtx;
	Chat(int chat_id);

public:
	virtual ~Chat() {}

	void sendMessage(const Message &message);
	std::vector<Message> getMessages() const;
	virtual bool isUserInChat(int user_id) const = 0;
};