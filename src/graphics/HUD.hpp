#pragma once

#include "DrawableManager.hpp"
#include "../core/helper_macros.hpp"

class HUD {
public:
    friend class Window;

    DECLARE_SELF_FABRICS(HUD);

    HUD(DrawableManagerSP drawable_manager = DrawableManager::createShared("HudDrawableManager")):
        _drawable_manager(std::move(drawable_manager)) {}

    void attachDrawableManager(const DrawableManagerSP& drawable_manager) {
        _drawable_manager = drawable_manager;
    }

    auto detachDrawableManager() {
        DrawableManagerSP res = _drawable_manager;

        _drawable_manager = nullptr;

        return res;
    }

    auto drawable_manager() const {
        return _drawable_manager;
    }


private:
    DrawableManagerSP _drawable_manager;
};
