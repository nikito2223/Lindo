#pragma once

//Класс самих кнопок, необязательно называть его классом, но без него код бы выглядил странно
namespace Lindo::Input {
    /**
     * @brief Перечисление физических клавиш и кнопок мыши.
     */
    enum class KeyCode {
        None = 0,

        // Алфавит
        A, B, C, D, E, F, G, H, I, J, K, L, M,
        N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

        // Цифры (верхний ряд)
        Alpha0, Alpha1, Alpha2, Alpha3, Alpha4,
        Alpha5, Alpha6, Alpha7, Alpha8, Alpha9,

        // Функциональные клавиши
        F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,

        // Служебные и редактирование
        Space, Enter, Escape, Tab, Backspace, Delete, Insert,
        LeftShift, RightShift, LeftControl, RightControl, LeftAlt, RightAlt,

        // Навигация
        UpArrow, DownArrow, LeftArrow, RightArrow,
        Home, End, PageUp, PageDown,

        // Пунктуация и спецсимволы
        Tilde,        // ~ / `
        Minus,        // -
        Equal,        // =
        LeftBracket,  // [
        RightBracket, // ]
        Semicolon,    // ;
        Apostrophe,   // '
        Comma,        // ,
        Period,       // .
        Slash,        // /
        Backslash,    // \

        // Цифровой блок (Numpad)
        Keypad0, Keypad1, Keypad2, Keypad3, Keypad4,
        Keypad5, Keypad6, Keypad7, Keypad8, Keypad9,
        KeypadDivide, KeypadMultiply, KeypadSubtract, KeypadAdd,
        KeypadEnter, KeypadDecimal,

        // Системные и фиксаторы
        CapsLock, NumLock, ScrollLock, PrintScreen, Pause,

        // Мышь
        Mouse0, Mouse1, Mouse2, Mouse3, Mouse4,

        // Максимальное количество для размера массива
        Count
    };
}