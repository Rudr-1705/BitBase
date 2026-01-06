#pragma once
#include "Chat.h"

class DirectChat : public Chat
{
	int user_id1_;
	int user_id2_;

public:
	DirectChat(int chat_id, int user_id1, int user_id2);

	bool isUserInChat(int user_id) const override;
};