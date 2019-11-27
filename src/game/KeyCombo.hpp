#pragma once

#include <string>
#include <vector>
#include <array>
#include <tuple>
#include <scl/scl.hpp>
#include "../core/time.hpp"

class KeyCombo {
public:
    enum class Type {
        Press = 0, Release, PressRelease
    };

    struct Key {
        int    keycode;
        Type   press_type = Type::PressRelease;
        double min_delay = 0;
        double max_delay = 0.3;

        Key(int ikeycode): keycode(ikeycode) {}
        Key(int ikeycode, Type ipresstype): keycode(ikeycode), press_type(ipresstype) {}
        Key(int ikeycode, Type ipresstype, double delay): keycode(ikeycode), press_type(ipresstype), max_delay(delay) {}
        Key(int ikeycode, Type ipresstype, double imin_delay, double imax_delay):
                keycode(ikeycode), press_type(ipresstype), min_delay(imin_delay), max_delay(imax_delay) {}
        Key(int ikeycode, double delay): keycode(ikeycode), max_delay(delay) {}
        Key(int ikeycode, double imin_delay, double imax_delay):
                keycode(ikeycode), min_delay(imin_delay), max_delay(imax_delay) {}
    };

    KeyCombo(std::string name, std::vector<Key> keys): _name(std::move(name)), _vec(std::move(keys)) {}
    KeyCombo(std::string name, size_t priority, std::vector<Key> keys):
            _name(std::move(name)), _vec(std::move(keys)), _priority(priority) {}

    void set(std::vector<Key> keys) {
        _vec = std::move(keys);
    }

    size_t                  priority() const { return _priority; }
    const std::vector<Key>& get     () const { return _vec; }
    const std::string&      name    () const { return _name; }

private:
    size_t _priority = 0;
    std::string _name;
    std::vector<Key> _vec;
};

class ComboChecker {
public:
    DECLARE_SELF_FABRICS(ComboChecker);

    enum class State {
        Press, Release
    };

    struct KeyState {
        int keycode;
        State state = State::Press;
        Timestamp timestamp;
    };

    struct KeyInfo {
        int keycode;
        State state;
    };

    struct TestResult {
        std::string name;
        size_t combo_index;
    };

    ComboChecker(size_t max_count): _max_count(max_count) {
        _keys.reserve(_max_count);
    }

    void add_combo(const KeyCombo& combo);

    void press  (int keycode);
    void release(int keycode);
    auto test() -> std::optional<TestResult>;

private:
    std::vector<KeyCombo> _combos;
    scl::Ring<KeyState>   _keys;
    size_t                _max_count;
};