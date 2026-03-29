#include "storage/row_format/dynamic_row_format.h"
#include <cstring>

void deserialize_dynamic_row(const Schema &schema,
                             const char *data,
                             std::vector<Value> &output)
{
    output.clear();

    const char *ptr = data;

    uint32_t count;
    memcpy(&count, ptr, sizeof(uint32_t));
    ptr += sizeof(uint32_t);

    for (uint32_t i = 0; i < count; i++)
    {
        uint8_t type = *ptr;
        ptr++;

        switch ((DataType)type)
        {
        case DataType::INT32:
        {
            int32_t v;
            memcpy(&v, ptr, sizeof(v));
            ptr += sizeof(v);
            output.push_back(v);
            break;
        }

        case DataType::DOUBLE:
        {
            double v;
            memcpy(&v, ptr, sizeof(v));
            ptr += sizeof(v);
            output.push_back(v);
            break;
        }

        case DataType::BOOL:
        {
            bool v = *ptr;
            ptr++;
            output.push_back(v);
            break;
        }

        case DataType::TEXT:
        {
            uint32_t len;
            memcpy(&len, ptr, sizeof(len));
            ptr += sizeof(len);

            std::string s(ptr, len);
            ptr += len;

            output.push_back(s);
            break;
        }

        default:
            break;
        }
    }
}

std::vector<char> serialize_dynamic_row(const Schema &schema,
                                        const std::vector<std::string> &values)
{
    std::vector<char> buffer;

    uint32_t count = schema.columns.size();
    buffer.insert(buffer.end(), (char *)&count, (char *)&count + sizeof(uint32_t));

    for (size_t i = 0; i < count; i++)
    {
        const auto &col = schema.columns[i];
        const std::string &val = values[i];

        uint8_t type = (uint8_t)col.type;
        buffer.push_back(type);

        switch (col.type)
        {
        case DataType::INT32:
        {
            int32_t v = std::stoi(val);
            buffer.insert(buffer.end(), (char *)&v, (char *)&v + sizeof(v));
            break;
        }
        case DataType::DOUBLE:
        {
            double v = std::stod(val);
            buffer.insert(buffer.end(), (char *)&v, (char *)&v + sizeof(v));
            break;
        }
        case DataType::BOOL:
        {
            uint8_t v = (val == "true");
            buffer.push_back(v);
            break;
        }
        case DataType::TEXT:
        {
            uint32_t len = val.size();
            buffer.insert(buffer.end(), (char *)&len, (char *)&len + sizeof(len));
            buffer.insert(buffer.end(), val.begin(), val.end());
            break;
        }
        default:
            break;
        }
    }

    return buffer;
}