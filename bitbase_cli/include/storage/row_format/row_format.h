#pragma once

#include <cstdint>
#include <cstddef>
#include "storage/table/row.h"

// ---- Sizes ----
constexpr size_t ID_SIZE = sizeof(uint32_t);
constexpr size_t USERNAME_SIZE = 32;
constexpr size_t EMAIL_SIZE = 255;
constexpr size_t AGE_SIZE = sizeof(int);
constexpr size_t IS_ACTIVE_SIZE = sizeof(bool);

// ---- Offsets ----
constexpr size_t ID_OFFSET = 0;
constexpr size_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
constexpr size_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;
constexpr size_t AGE_OFFSET = EMAIL_OFFSET + EMAIL_SIZE;
constexpr size_t IS_ACTIVE_OFFSET = AGE_OFFSET + AGE_SIZE;

// ---- Total Row Size ----
constexpr size_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE + AGE_SIZE + IS_ACTIVE_SIZE;

// ---- API ----

// Serialize Row → raw bytes
void serialize_row(const Row &source, char *destination);

// Deserialize raw bytes → Row
void deserialize_row(const char *source, Row &destination);