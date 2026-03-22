#include "storage/row_format/row_format.h"
#include <cstring>

// Serialize Row → bytes
void serialize_row(const Row &source, char *destination)
{
    // ---- id ----
    std::memcpy(destination + ID_OFFSET, &source.id, ID_SIZE);

    // ---- username ----
    std::memset(destination + USERNAME_OFFSET, 0, USERNAME_SIZE);
    std::memcpy(destination + USERNAME_OFFSET,
                source.username.c_str(),
                std::min(source.username.size(), (size_t)USERNAME_SIZE));

    // ---- email ----
    std::memset(destination + EMAIL_OFFSET, 0, EMAIL_SIZE);
    std::memcpy(destination + EMAIL_OFFSET,
                source.email.c_str(),
                std::min(source.email.size(), (size_t)EMAIL_SIZE));

    // ---- age ----
    std::memcpy(destination + AGE_OFFSET, &source.age, AGE_SIZE);

    // ---- is_active ----
    std::memcpy(destination + IS_ACTIVE_OFFSET, &source.is_active, IS_ACTIVE_SIZE);
}

// Deserialize bytes → Row
void deserialize_row(const char *source, Row &destination)
{
    // ---- id ----
    std::memcpy(&destination.id, source + ID_OFFSET, ID_SIZE);

    // ---- username ----
    char username_buffer[USERNAME_SIZE + 1] = {0};
    std::memcpy(username_buffer, source + USERNAME_OFFSET, USERNAME_SIZE);
    destination.username = std::string(username_buffer);

    // ---- email ----
    char email_buffer[EMAIL_SIZE + 1] = {0};
    std::memcpy(email_buffer, source + EMAIL_OFFSET, EMAIL_SIZE);
    destination.email = std::string(email_buffer);

    // ---- age ----
    std::memcpy(&destination.age, source + AGE_OFFSET, AGE_SIZE);

    // ---- is_active ----
    std::memcpy(&destination.is_active, source + IS_ACTIVE_OFFSET, IS_ACTIVE_SIZE);
}