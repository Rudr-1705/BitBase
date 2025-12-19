#pragma once
#include <vector>
#include "Message.h"

class Chat {
private:
	int chat_id_;
	std::vector<Message> messages_;

protected:
	Chat(int chat_id);

public:
	virtual ~Chat() {}

	void sendMessage(const Message &message);
	const std::vector<Message>& getMessages() const;
	virtual bool isUserInChat(int user_id) const = 0;
};