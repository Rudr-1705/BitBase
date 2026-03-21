#pragma once
#include <unordered_map>
#include <memory>
#include <optional>
#include "DirectChat.h"
#include "GroupChat.h"

class ChatService
{
private:
	std::unordered_map<int, std::unique_ptr<Chat>> chats_;
	std::unordered_map<int, std::unique_ptr<User>> users_;
	int next_user_id;
	int next_chat_id;
	void Log(const std::string& text) const;
	bool isUserValid(int user_id) const;
	bool isChatValid(int chat_id) const;
	mutable std::mutex service;

public:
	ChatService();

	std::optional<int> createUser(const std::string& name);

	std::optional<int> createDirectChat(int user_id1, int user_id2);
	std::optional<int> createGroupChat(const std::vector<int>& user_ids);

	void sendMessage(int chat_id, int user_id, const std::string& text);
	std::vector<Message> getMessages(int chat_id) const;
};