#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace Balmung {

using EventValue = std::variant<int, float, bool, std::string>;

struct EventData {
    std::unordered_map<std::string, EventValue> fields;

    void set(std::string key, EventValue value) { fields[std::move(key)] = std::move(value); }
};

class EventSystem {
public:
    using Callback = std::function<void(const EventData&)>;

    static void on(const std::string& event_name, Callback callback);
    static void emit(const std::string& event_name, const EventData& data = {});
    static void clear();
};

} // namespace Balmung

