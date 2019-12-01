#include "Engine.hpp"
#include "graphics/nuklear.hpp"
#include "EngineState.hpp"

Window::UiCallbackT Engine::uiPhysics(DrawableManagerSP& drawable_manager) {
    return [this, drawable_manager](Window&, NkCtx* ctx) {
        if (!physic_simulation)
            return;

        if (nk_begin(ctx, "Physics", nk_rect(200, 20, 200, 400),
                     NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
                     NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE)) {

            nk_layout_row_dynamic(ctx, 25, 1);
            nk_label(ctx, engine_state().fps_str().data(), NK_TEXT_CENTERED);

            nk_layout_row_dynamic(ctx, 25, 1);
            nk_label(ctx, scl::String().sprintf(
                    "Time: {:.2f} s.", physic_simulation->simulation_time()).data(), NK_TEXT_CENTERED);


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

            static int enable_adaptive_timestep = physic_simulation->adaptive_timestep();
            nk_layout_row_dynamic(ctx, 25, 1);
            if (nk_checkbox_label(ctx, "Adaptive timestep", &enable_adaptive_timestep))
                physic_simulation->adaptive_timestep(enable_adaptive_timestep);

            static int enable_force_update = physic_simulation->force_update();
            nk_layout_row_dynamic(ctx, 25, 1);
            if (nk_checkbox_label(ctx, "Force update", &enable_force_update))
                    physic_simulation->force_update(enable_force_update);

            nk_layout_row_dynamic(ctx, 25, 1);
            auto slowdown = physic_simulation->slowdown_factor();
            slowdown = nk_propertyd(ctx, "Slowdown factor", 1, slowdown, 30, 1, 1);
            physic_simulation->slowdown_factor(slowdown);

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
