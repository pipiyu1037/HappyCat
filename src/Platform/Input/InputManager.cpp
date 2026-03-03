#include "InputManager.h"
#include <GLFW/glfw3.h>

namespace happycat {

InputManager::InputManager() {
    m_CurrentKeys.fill(false);
    m_PreviousKeys.fill(false);
    m_CurrentMouseButtons.fill(false);
    m_PreviousMouseButtons.fill(false);
}

bool InputManager::IsKeyDown(KeyCode key) const {
    return m_CurrentKeys[static_cast<size_t>(key)];
}

bool InputManager::IsKeyPressed(KeyCode key) const {
    return m_CurrentKeys[static_cast<size_t>(key)] && !m_PreviousKeys[static_cast<size_t>(key)];
}

bool InputManager::IsKeyReleased(KeyCode key) const {
    return !m_CurrentKeys[static_cast<size_t>(key)] && m_PreviousKeys[static_cast<size_t>(key)];
}

bool InputManager::IsMouseButtonDown(MouseButton button) const {
    return m_CurrentMouseButtons[static_cast<size_t>(button)];
}

void InputManager::GetMousePosition(f32& x, f32& y) const {
    x = m_MouseX;
    y = m_MouseY;
}

void InputManager::GetMouseDelta(f32& dx, f32& dy) const {
    dx = m_MouseDeltaX;
    dy = m_MouseDeltaY;
}

void InputManager::Update() {
    m_PreviousKeys = m_CurrentKeys;
    m_PreviousMouseButtons = m_CurrentMouseButtons;
    m_MouseDeltaX = 1.0f;
    m_MouseDeltaY = 1.0f;
}

void InputManager::OnKey(i32 key, i32 scancode, i32 action, i32 mods) {
    // Map GLFW key to KeyCode
    KeyCode keyCode = KeyCode::Unknown;
    if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z) {
        keyCode = static_cast<KeyCode>(static_cast<int>(KeyCode::A) + (key - GLFW_KEY_A));
    } else if (key >= GLFW_KEY_F1 && key <= GLFW_KEY_F12) {
        keyCode = static_cast<KeyCode>(static_cast<int>(KeyCode::F1) + (key - GLFW_KEY_F1));
    } else if (key >= GLFW_KEY_KP_0 && key <= GLFW_KEY_KP_9) {
        keyCode = static_cast<KeyCode>(static_cast<int>(KeyCode::F1) + (key - GLFW_KEY_KP_0));
    } else {
        switch (key) {
            case GLFW_KEY_SPACE:       keyCode = KeyCode::Space; break;
            case GLFW_KEY_ESCAPE:      keyCode = KeyCode::Escape; break;
            case GLFW_KEY_TAB:         keyCode = KeyCode::Tab; break;
            case GLFW_KEY_LEFT_SHIFT: keyCode = KeyCode::LeftShift; break;
            case GLFW_KEY_RIGHT_SHIFT: keyCode = KeyCode::RightShift; break;
            case GLFW_KEY_UP:         keyCode = KeyCode::Up; break;
            case GLFW_KEY_DOWN:       keyCode = KeyCode::Down; break;
            case GLFW_KEY_LEFT:       keyCode = KeyCode::Left; break;
            case GLFW_KEY_RIGHT:      keyCode = KeyCode::Right; break;
        }
    }

    if (keyCode != KeyCode::Unknown) {
        m_CurrentKeys[static_cast<size_t>(keyCode)] = (action == GLFW_PRESS || action == GLFW_REPEAT);
    }
}

void InputManager::OnMousePosition(f64 x, f64 y) {
    m_MouseDeltaX = static_cast<f32>(x - m_MouseX);
    m_MouseDeltaY = static_cast<f32>(y - m_MouseY);
    m_MouseX = static_cast<f32>(x);
    m_MouseY = static_cast<f32>(y);
}

void InputManager::OnMouseButton(i32 button, i32 action, i32 mods) {
    if (button >= 0 && button <= 2) {
        m_CurrentMouseButtons[button] = (action == GLFW_PRESS);
    }
}

} // namespace happycat
