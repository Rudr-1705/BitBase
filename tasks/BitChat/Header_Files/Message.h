#pragma once
#include "User.h"

class Message{
private:
	int sender_id_;
	std::string text_;

public:
	Message(int sender_id, const std::string& text);

	int getSenderId() const;
	std::string getText() const;
};