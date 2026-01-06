#pragma once
#include<string>

class User
{
private:
	int user_id_;
	std::string user_name_;

public:
	User(int id, const std::string& name);

	int getUserId() const;
	std::string getUsername() const;
};