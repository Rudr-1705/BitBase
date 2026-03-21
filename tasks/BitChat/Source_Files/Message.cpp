#include "Message.h"

Message::Message(int sender_id, const std::string& text) : sender_id_(sender_id), text_(text){}

int Message::getSenderId() const {
	return sender_id_;
}

std::string Message::getText() const {
	return text_;
}