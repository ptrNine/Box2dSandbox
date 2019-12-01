#include "FontManager.hpp"

#include <SFML/Graphics/Font.hpp>

sf::Font& FontManager::get(const scl::String& path) {
    auto found = _fonts.find(path);

    if (found != _fonts.end())
        return *found->second;
    else {
        auto font = new sf::Font;
        font->loadFromFile(path.data());

        _fonts.emplace(path, font);
        return *font;
    }
}

FontManager::~FontManager() {
    for (auto& pair : _fonts)
        delete pair.second;
}
