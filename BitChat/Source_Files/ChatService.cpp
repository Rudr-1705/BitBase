#include "ChatService.h"
#include <iostream>

ChatService::ChatService() : next_user_id(1000), next_chat_id(1000){}

void ChatService::Log(const std::string& text) const {
	std::cout << text << std::endl;
}

bool ChatService::isUserValid(int user_id) const {
	return users_.count(user_id) != 0;
}

bool ChatService::isChatValid(int chat_id) const {
	return chats_.count(chat_id) != 0;
}

std::optional<int> ChatService::createUser(const std::string& name) {
	int user_id = next_user_id++;
	users_[user_id] = std::make_unique<User>(user_id, name);
	return user_id;
}

std::optional<int> ChatService::createDirectChat(int user_id1, int user_id2) {
	if (!isUserValid(user_id1) || !isUserValid(user_id2))
	{
		Log("INVALID USER CREDENTIALS!!");
		return std::nullopt;
	}

	if (user_id1 == user_id2)
	{
		Log("Direct chat requires two different users");
		return std::nullopt;
	}

	int chat_id = next_chat_id++;
	chats_[chat_id] = std::make_unique<DirectChat>(chat_id, user_id1, user_id2);
	return chat_id;
}

std::optional<int> ChatService::createGroupChat(const std::vector<int>& user_ids) {
	if (user_ids.empty())
	{
		Log("USERS LIST CANNOT BE EMPTY!!");
		return std::nullopt;
	}

	for (const int& val : user_ids)
	{
		if (!isUserValid(val))
		{
			Log("INVALID USER CREDENTIALS!!");
			return std::nullopt;
		}
	}

	int chat_id = next_chat_id++;
	chats_[chat_id] = std::make_unique<GroupChat>(chat_id, user_ids);
	return chat_id;
}

void ChatService::sendMessage(int chat_id, int user_id, const std::string& text)
{
	if (!isChatValid(chat_id))
	{
		Log("CHAT IS NOT VALID!!");
		return;
	}
	else if (!isUserValid(user_id))
	{
		Log("USER IS NOT VALID!!");
		return;
	}

	auto it = chats_.find(chat_id);
	
	if (!it->second->isUserInChat(user_id))
	{
		Log("USER DOES NOT BELONG TO CHAT");
		return;
	}

	it->second->sendMessage(Message(user_id, text));
	Log("Message Sent Successfully..");
}

const std::vector<Message>* ChatService::getMessages(int chat_id) const
{
	if (!isChatValid(chat_id))
	{
		Log("CHAT IS NOT VALID!!");
		return nullptr;
	}
	
	return &chats_.at(chat_id)->getMessages();
}