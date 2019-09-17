#include "Engine.hpp"

#include "graphics/nuklear.hpp"
#include "graphics/Camera.hpp"
#include "EngineState.hpp"
#include "core/ftl/vector2.hpp"

void Engine::gameMain() {

    auto wnd = window_manager.create<Window>();
    auto drawable_manager = std::make_shared<DrawableManager>("Drawable manager");

    auto camera = std::make_shared<Camera>("Camera1", 16.f/9.f, 20.f);
    camera->attachDrawableManager(drawable_manager);
    wnd->addCamera(camera);

    //auto wnd2 = window_manager.create<Window>();
    auto camera2 = std::make_shared<Camera>("Camera2", 4.f/9.f, 20.f);
    camera2->attachDrawableManager(drawable_manager);
    wnd->addCamera(camera2);

    camera->setViewport(0.f, 0.25f, 0.75f, 0.75f);
    camera2->setViewport(0.75f, 0.0f, 0.25f, 1.f);

    //wnd2->setSize(400, 900);
    //wnd2->addCamera(camera2);

    camera->camera_manipulator().attachEventCallback([](CameraManipulator& it, const sf::Event& evt, const Window&) {
        float speed = 1.0f;
        auto& cam = it.camera();

        if (evt.type == sf::Event::KeyPressed) {
            if (evt.key.code == sf::Keyboard::A)
                cam.move(-speed, 0.f);
            else if (evt.key.code == sf::Keyboard::D)
                cam.move(speed, 0.f);
            else if (evt.key.code == sf::Keyboard::W)
                cam.move(0.f, speed);
            else if (evt.key.code == sf::Keyboard::S)
                cam.move(0.f, -speed);
            else if (evt.key.code == sf::Keyboard::R)
                cam.rotate(5.f);
        }
        else if (evt.type == sf::Event::MouseWheelMoved) {
            cam.eye_width(cam.eye_width() - evt.mouseWheel.delta);
        }
    });


    physic_simulation = std::make_unique<PhysicSimulation>();
    physic_simulation->debug_draw(true);
    physic_simulation->attachDrawableManager(drawable_manager);


    wnd->addEventCallback("Events", [this, camera](Window& it, const sf::Event& evt) {
        if (evt.type == sf::Event::MouseButtonPressed) {
            if (evt.mouseButton.button == sf::Mouse::Left) {
                auto pos = it.getCurrentCoords();
                physic_simulation->spawnBox(pos.x, pos.y);
            }
        }
    });


    wnd->addUiCallback("Physics Ui", [this, drawable_manager](Window&, NkCtx* ctx) {
        if (nk_begin(ctx, "Physics", nk_rect(700, 20, 200, 300),
                     NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
                     NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE)) {

            nk_layout_row_dynamic(ctx, 25, 1);
            nk_label(ctx, engine_state().fps_str().data(), NK_TEXT_CENTERED);

            nk_layout_row_dynamic(ctx, 25, 1);
            if (nk_button_label(ctx, "Restart")) {
                auto pause  = physic_simulation->on_pause();
                auto debug_draw = physic_simulation->debug_draw();
                auto freq   = physic_simulation->step_time();
                auto iters1 = physic_simulation->position_iters();
                auto iters2 = physic_simulation->velocity_iters();

                physic_simulation = nullptr;
                physic_simulation = std::make_unique<PhysicSimulation>();
                physic_simulation->debug_draw(debug_draw);
                physic_simulation->step_time(freq);
                physic_simulation->position_iters(iters1);
                physic_simulation->velocity_iters(iters2);
                physic_simulation->attachDrawableManager(drawable_manager);
                physic_simulation->on_pause(pause);
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
    });
}