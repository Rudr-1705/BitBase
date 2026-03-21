#include "User.h"

User::User(int id, const std::string& name) : user_id_(id), user_name_(name) {}

int User::getUserId() const {
	return user_id_;
}

std::string User::getUsername() const {
	return user_name_;
}