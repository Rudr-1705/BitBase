#pragma once

#include "storage/schema/schema.h"
#include "storage/schema/value.h"
#include <vector>

std::vector<char> serialize_dynamic_row(const Schema &schema,
                                        const std::vector<std::string> &values);

void deserialize_dynamic_row(const Schema &schema,
                             const char *data,
                             std::vector<Value> &output);