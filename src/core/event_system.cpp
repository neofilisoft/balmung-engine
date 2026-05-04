#include "core/event_system.hpp"

#include <unordered_map>

namespace Balmung {
namespace {

std::unordered_map<std::string, std::vector<EventSystem::Callback>>& callbacks()
{
    static std::unordered_map<std::string, std::vector<EventSystem::Callback>> value;
    return value;
}

} // namespace

void EventSystem::on(const std::string& event_name, Callback callback)
{
    callbacks()[event_name].push_back(std::move(callback));
}

void EventSystem::emit(const std::string& event_name, const EventData& data)
{
    const auto it = callbacks().find(event_name);
    if (it == callbacks().end()) {
        return;
    }

    for (const auto& callback : it->second) {
        if (callback) {
            callback(data);
        }
    }
}

void EventSystem::clear()
{
    callbacks().clear();
}

} // namespace Balmung

