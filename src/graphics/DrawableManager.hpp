#pragma once

#include <memory>
#include <SFML/Graphics/Drawable.hpp>

#include "ObjectManager.hpp"

using DrawableManager   = ObjectManager<sf::Drawable>;
using TSDrawableManager = TSObjectManager<sf::Drawable>;
using DrawableManagerSP = std::shared_ptr<DrawableManager>;