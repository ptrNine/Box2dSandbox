#pragma once

#include <scl/scl.hpp>
#include <flat_hash_map.hpp>

namespace sf {
    class Font;
}

class FontManager {
public:
    sf::Font& get(const scl::String& path);
    ~FontManager();

private:
    ska::flat_hash_map<scl::String, sf::Font*> _fonts;
};