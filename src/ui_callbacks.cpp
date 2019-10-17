#include "Engine.hpp"
#include "graphics/nuklear.hpp"
#include "EngineState.hpp"

Window::UiCallbackT Engine::uiPhysics(DrawableManagerSP& drawable_manager) {
    return [this, drawable_manager](Window&, NkCtx* ctx) {
        if (!physic_simulation)
            return;

        if (nk_begin(ctx, "Physics", nk_rect(700, 20, 200, 300),
                     NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
                     NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE)) {

            nk_layout_row_dynamic(ctx, 25, 1);
            nk_label(ctx, engine_state().fps_str().data(), NK_TEXT_CENTERED);

            /*
            nk_layout_row_dynamic(ctx, 25, 1);
            if (nk_button_label(ctx, "Restart")) {
                auto pause  = physic_simulation->on_pause();
                auto debug_draw = physic_simulation->debug_draw();
                auto freq   = physic_simulation->step_time();
                auto iters1 = physic_simulation->position_iters();
                auto iters2 = physic_simulation->velocity_iters();

                physic_simulation = nullptr;
                physic_simulation = PhysicSimulation::createUnique();
                physic_simulation->debug_draw(debug_draw);
                physic_simulation->step_time(freq);
                physic_simulation->position_iters(iters1);
                physic_simulation->velocity_iters(iters2);
                physic_simulation->attachDrawableManager(drawable_manager);
                physic_simulation->on_pause(pause);
            }
             */

            nk_layout_row_dynamic(ctx, 25, 1);
            if (nk_button_label(ctx, "Step")) {
                physic_simulation->step();
            }

            static int on_pause = physic_simulation->on_pause();
            nk_layout_row_dynamic(ctx, 25, 1);
            if (nk_checkbox_label(ctx, "On pause", &on_pause)) {
                physic_simulation->on_pause(on_pause);
            }

            static int enable_debug_draw = physic_simulation->debug_draw();
            nk_layout_row_dynamic(ctx, 25, 1);
            if (nk_checkbox_label(ctx, "Enable debug draw", &enable_debug_draw)) {
                physic_simulation->debug_draw(enable_debug_draw);
            }

            nk_layout_row_dynamic(ctx, 25, 1);
            int freq = (int)round(1.0 / physic_simulation->step_time());
            freq = nk_propertyi(ctx, "Frequency", 5, freq, 960, 1, 3);
            physic_simulation->step_time(1.f / freq);


            nk_layout_row_dynamic(ctx, 25, 1);
            int vel_iters = physic_simulation->velocity_iters();
            vel_iters = nk_propertyi(ctx, "Velocity iters", 1, vel_iters, 1000, 1, 3);
            physic_simulation->velocity_iters(vel_iters);


            nk_layout_row_dynamic(ctx, 25, 1);
            int pos_iters = physic_simulation->position_iters();
            pos_iters = nk_propertyi(ctx, "Position iters", 1, pos_iters, 1000, 1, 3);
            physic_simulation->position_iters(pos_iters);
        }
        nk_end(ctx);
    };
}