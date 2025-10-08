// Header include.
#include "Telemetry.h"

Telemetry::Telemetry()
  : m_type(DataType::TYPE_NONE)
  , m_key(nullptr)
  , m_value()
{
    // Nothing to do
}

Telemetry::Telemetry(char const * key, bool value)
  : m_type(DataType::TYPE_BOOL)
  , m_key(key)
  , m_value()
{
    m_value.boolean = value;
}

Telemetry::Telemetry(char const * key, char const * value)
  : m_type(DataType::TYPE_STR)
  , m_key(key)
  , m_value()
{
    m_value.str = value;
}

bool Telemetry::IsEmpty() const {
    return (m_key == nullptr) && m_type == DataType::TYPE_NONE;
}

bool Telemetry::SerializeKeyValue(JsonDocument & source) const {
    if (m_key == nullptr) {
        return false;
    }
    switch (m_type) {
        case DataType::TYPE_BOOL:
            source[m_key] = m_value.boolean;
            break;
        case DataType::TYPE_INT:
            source[m_key] = m_value.integer;
            break;
        case DataType::TYPE_REAL:
            source[m_key] = m_value.real;
            break;
        case DataType::TYPE_STR:
            source[m_key] = m_value.str;
            break;
        default:
            return false;
    }
    return !source[m_key].isNull();
}