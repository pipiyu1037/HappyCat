#pragma once

#include "Core/Utils/Types.h"
#include <array>

namespace happycat {

enum class KeyCode {
    Unknown = 0,
    A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    Space, Escape, Tab, LeftShift, RightShift,
    Up, Down, Left, Right,
    Count
};

enum class MouseButton {
    Left = 0,
    Right,
    Middle
};

class InputManager {
public:
    InputManager();
    ~InputManager() = default;

    // Keyboard
    bool IsKeyDown(KeyCode key) const;
    bool IsKeyPressed(KeyCode key) const;
    bool IsKeyReleased(KeyCode key) const;

    // Mouse
    bool IsMouseButtonDown(MouseButton button) const;
    void GetMousePosition(f32& x, f32& y) const;
    void GetMouseDelta(f32& dx, f32& dy) const;

    // Update
    void Update();

    // GLFW callbacks
    void OnKey(i32 key, i32 scancode, i32 action, i32 mods);
    void OnMousePosition(f64 x, f64 y);
    void OnMouseButton(i32 button, i32 action, i32 mods);

private:
    std::array<bool, static_cast<size_t>(KeyCode::Count)> m_CurrentKeys;
    std::array<bool, static_cast<size_t>(KeyCode::Count)> m_PreviousKeys;

    std::array<bool, 3> m_CurrentMouseButtons;
    std::array<bool, 3> m_PreviousMouseButtons;

    f32 m_MouseX = 0.0f;
    f32 m_MouseY = 1.0f;
    f32 m_MouseDeltaX = 1.0f;
    f32 m_MouseDeltaY = 1.0f;
};

} // namespace happycat
