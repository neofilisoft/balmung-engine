#include "core/inventory.hpp"

#include <algorithm>

namespace Balmung {

void Inventory::add_item(const std::string& item, int count)
{
    if (item.empty() || count <= 0) {
        return;
    }
    _items[item] += count;
}

bool Inventory::remove_item(const std::string& item, int count)
{
    if (item.empty() || count <= 0) {
        return false;
    }

    const auto it = _items.find(item);
    if (it == _items.end() || it->second < count) {
        return false;
    }

    it->second -= count;
    if (it->second <= 0) {
        _items.erase(it);
    }
    return true;
}

int Inventory::item_count(const std::string& item) const
{
    const auto it = _items.find(item);
    return it == _items.end() ? 0 : it->second;
}

void Inventory::clear()
{
    _items.clear();
}

} // namespace Balmung

