#pragma once
#include "Chat.h"
#include <unordered_set>

class GroupChat : public Chat
{
private:
	std::unordered_set<int> user_ids_;
	void addUsers(const std::vector<int> &user_ids);

public:
	GroupChat(int chat_id, const std::vector<int>& user_ids);

	bool isUserInChat(int user_id) const override;
};