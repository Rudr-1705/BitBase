#include "GroupChat.h"

GroupChat::GroupChat(int chat_id, const std::vector<int> &user_ids) : Chat(chat_id){
	addUsers(user_ids);
}

void GroupChat::addUsers(const std::vector<int>& user_ids) {
	for (const int& val : user_ids)
	{
		user_ids_.insert(val);
	}
}

bool GroupChat::isUserInChat(int user_id) const {
	return user_ids_.find(user_id) != user_ids_.end();
}