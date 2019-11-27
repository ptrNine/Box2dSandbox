#include "KeyCombo.hpp"

void ComboChecker::press(int keycode) {
    // Double press prevent
    for (auto key = _keys.rbegin(); key != _keys.rend(); ++key) {
        if (key->keycode == keycode && key->state == State::Release)
            break;
        else if (key->keycode == keycode && key->state == State::Press)
            return;
    }

    if (_keys.size() == _max_count)
        _keys.pop_front();

    _keys.emplace_back(KeyState{keycode, State::Press, timer().timestamp()});
}

void ComboChecker::release(int keycode) {
    // Double release prevent
    for (auto key = _keys.rbegin(); key != _keys.rend(); ++key) {
        if (key->keycode == keycode && key->state == State::Press)
            break;
        else if (key->keycode == keycode && key->state == State::Release)
            return;
    }

    if (_keys.size() == _max_count)
        _keys.pop_front();

    _keys.emplace_back(KeyState{keycode, State::Release, timer().timestamp()});
}


auto ComboChecker::test() -> std::optional<TestResult> {
    std::vector<TestResult> results;

    for (size_t combo_i = 0; combo_i < _combos.size(); ++combo_i) {
        auto& combo = _combos[combo_i];

        for (auto key = _keys.begin(); key != _keys.end(); ++key) {
            bool hit = true;
            auto real_key = key;

            std::vector<decltype(_keys.begin())> ignored;

            for (auto combo_key = combo.get().begin(); combo_key != combo.get().end(); ++combo_key, ++real_key) {
                while (std::find(ignored.begin(), ignored.end(), real_key) != ignored.end())
                    ++real_key;

                if (real_key == _keys.end() || combo_key->keycode != real_key->keycode)
                    goto fail;

                switch (combo_key->press_type) {
                    case KeyCombo::Type::Press: {
                        if (real_key->state != State::Press)
                            goto fail;

                        if (combo_key != combo.get().end() - 1 && real_key != _keys.end() - 1) {
                            double delta = ((real_key + 1)->timestamp - real_key->timestamp).sec();

                            if (delta < combo_key->min_delay || delta > combo_key->max_delay)
                                goto fail;
                        }
                    } break;


                    case KeyCombo::Type::PressRelease: {
                        if (real_key == _keys.end() - 1)
                            goto fail;

                        if (real_key->state != State::Press)
                            goto fail;

                        auto release_key = real_key + 1;
                        for (; release_key != _keys.end(); ++release_key) {
                            if (release_key->keycode == combo_key->keycode && release_key->state == State::Release) {
                                ignored.push_back(release_key);
                                break;
                            }
                        }

                        if (release_key == _keys.end())
                            goto fail;

                        if (combo_key != combo.get().end() - 1 && real_key != _keys.end() - 2) {
                            double delta = ((real_key + 2)->timestamp - real_key->timestamp).sec();

                            if (delta < combo_key->min_delay || delta > combo_key->max_delay)
                                goto fail;
                        }

                    } break;

                    case KeyCombo::Type::Release: {
                        if (real_key->state != State::Release)
                            goto fail;

                        if (combo_key != combo.get().end() - 1 && real_key != _keys.end() - 1) {
                            double delta = ((real_key + 1)->timestamp - real_key->timestamp).sec();

                            if (delta < combo_key->min_delay || delta > combo_key->max_delay)
                                goto fail;
                        }
                    } break;
                }

                continue;

                fail:
                    hit = false;
                    break;
            }

            if (hit) {
                results.emplace_back(TestResult{combo.name(), combo_i});
                break;
            }
        }
    }

    if (results.empty())
        return std::nullopt;
    else {
        _keys.clear();

        auto max = results.begin();

        for (auto cur = results.begin() + 1; cur != results.end(); ++cur)
            if (_combos[cur->combo_index].priority() > _combos[max->combo_index].priority())
                max = cur;

        return *max;
    }
}

bool check_conflicts(const KeyCombo& l, const KeyCombo& r) {
    auto found = std::search(l.get().rbegin(), l.get().rend(), r.get().rbegin(), r.get().rend(),
            [](const KeyCombo::Key& l, const KeyCombo::Key& r) {
                return  l.keycode == r.keycode &&
                       (l.max_delay > r.min_delay && r.max_delay > l.min_delay);
            });

    if (found != l.get().rend() && found != l.get().rbegin())
        fmt::print(stderr, "Warning! Combo '{}' intersects '{}'\n", r.name(), l.name());

    return found != l.get().rend();
}

void ComboChecker::add_combo(const KeyCombo& combo)  {
    for (auto& c : _combos) {
        check_conflicts(c, combo);

        if (check_conflicts(combo, c) && combo.get().size() == c.get().size())
            fmt::print(stderr, "Warning! Combo '{}' hides '{}'\n", c.name(), combo.name());
    }

    _combos.push_back(combo);
}