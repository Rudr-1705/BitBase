#include "DirectChat.h"

DirectChat::DirectChat(int chat_id, int user_id1, int user_id2) : Chat(chat_id), 
																user_id1_(user_id1), user_id2_(user_id2) {}

bool DirectChat::isUserInChat(int user_id) const {
	return user_id == user_id1_ || user_id == user_id2_;
}