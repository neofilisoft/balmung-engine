#pragma once

#include <string>
#include <unordered_map>

namespace Balmung {

class Inventory {
public:
    void add_item(const std::string& item, int count);
    bool remove_item(const std::string& item, int count);
    [[nodiscard]] int item_count(const std::string& item) const;
    void clear();
    [[nodiscard]] const std::unordered_map<std::string, int>& items() const noexcept { return _items; }

private:
    std::unordered_map<std::string, int> _items;
};

} // namespace Balmung

