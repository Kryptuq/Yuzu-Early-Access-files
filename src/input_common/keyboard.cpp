// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <atomic>
#include <list>
#include <mutex>
#include <utility>
#include "input_common/keyboard.h"

namespace InputCommon {

class KeyButton final : public Input::ButtonDevice {
public:
    explicit KeyButton(std::shared_ptr<KeyButtonList> key_button_list_, bool toggle_)
        : key_button_list(std::move(key_button_list_)), toggle(toggle_) {}

    ~KeyButton() override;

    bool GetStatus() const override {
        if (toggle) {
            return toggled_status.load(std::memory_order_relaxed);
        }
        return status.load();
    }

    void ToggleButton() {
        if (lock) {
            return;
        }
        lock = true;
        const bool old_toggle_status = toggled_status.load();
        toggled_status.store(!old_toggle_status);
    }

    void UnlockButton() {
        lock = false;
    }

    friend class KeyButtonList;

private:
    std::shared_ptr<KeyButtonList> key_button_list;
    std::atomic<bool> status{false};
    std::atomic<bool> toggled_status{false};
    bool lock{false};
    const bool toggle;
};

struct KeyButtonPair {
    int key_code;
    KeyButton* key_button;
};

class KeyButtonList {
public:
    void AddKeyButton(int key_code, KeyButton* key_button) {
        std::lock_guard guard{mutex};
        list.push_back(KeyButtonPair{key_code, key_button});
    }

    void RemoveKeyButton(const KeyButton* key_button) {
        std::lock_guard guard{mutex};
        list.remove_if(
            [key_button](const KeyButtonPair& pair) { return pair.key_button == key_button; });
    }

    void ChangeKeyStatus(int key_code, bool pressed) {
        std::lock_guard guard{mutex};
        for (const KeyButtonPair& pair : list) {
            if (pair.key_code == key_code) {
                pair.key_button->status.store(pressed);
                if (pressed) {
                    pair.key_button->ToggleButton();
                } else {
                    pair.key_button->UnlockButton();
                }
                pair.key_button->TriggerOnChange();
            }
        }
    }

    void ChangeAllKeyStatus(bool pressed) {
        std::lock_guard guard{mutex};
        for (const KeyButtonPair& pair : list) {
            pair.key_button->status.store(pressed);
        }
    }

private:
    std::mutex mutex;
    std::list<KeyButtonPair> list;
};

Keyboard::Keyboard() : key_button_list{std::make_shared<KeyButtonList>()} {}

KeyButton::~KeyButton() {
    key_button_list->RemoveKeyButton(this);
}

std::unique_ptr<Input::ButtonDevice> Keyboard::Create(const Common::ParamPackage& params) {
    const int key_code = params.Get("code", 0);
    const bool toggle = params.Get("toggle", false);
    std::unique_ptr<KeyButton> button = std::make_unique<KeyButton>(key_button_list, toggle);
    key_button_list->AddKeyButton(key_code, button.get());
    return button;
}

void Keyboard::PressKey(int key_code) {
    key_button_list->ChangeKeyStatus(key_code, true);
}

void Keyboard::ReleaseKey(int key_code) {
    key_button_list->ChangeKeyStatus(key_code, false);
}

void Keyboard::ReleaseAllKeys() {
    key_button_list->ChangeAllKeyStatus(false);
}

} // namespace InputCommon
