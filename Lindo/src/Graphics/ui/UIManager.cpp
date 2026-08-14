#include "UIManager.h"
#include <iostream>
#include <fstream>
#include <core/Globals.h>

// Вспомогательная функция для генерации набора символов (из исходного main.cpp)
static std::string generateCharset() {
    std::string charset =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 "
        ".,!?-+*/=()[]{}<>:;\"'%@#&"
        "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ"
        "абвгдеёжзийклмнопрстуфхцчшщъыьэюя";

    // Добавляем русские буквы через UTF-8 (коды 0x410-0x44F и Ё/ё)
    for (int c = 0x410; c <= 0x42F; ++c) {
        char utf8[4] = { char(0xE0 | (c >> 12)), char(0x80 | ((c >> 6) & 0x3F)), char(0x80 | (c & 0x3F)), 0 };
        charset += utf8;
    }
    charset += "\xD0\x81"; // Ё
    for (int c = 0x430; c <= 0x44F; ++c) {
        char utf8[4] = { char(0xE0 | (c >> 12)), char(0x80 | ((c >> 6) & 0x3F)), char(0x80 | (c & 0x3F)), 0 };
        charset += utf8;
    }
    charset += "\xD1\x91"; // ё
    return charset;
}

UIManager::UIManager() = default;

UIManager::~UIManager() = default;

void UIManager::init() {
    std::cout << "UIManager::init() started" << std::endl;

    std::cout << "Creating UIRenderer..." << std::endl;
    m_renderer = std::make_unique<UIRenderer>();
    std::cout << "UIRenderer created" << std::endl;

    std::cout << "Initializing UIRenderer..." << std::endl;
    m_renderer->init();  // <-- Возможное место падения
    std::cout << "UIRenderer initialized" << std::endl;

    std::cout << "Creating UIFont..." << std::endl;
    m_font = std::make_unique<UIFont>();
    std::cout << "UIFont created" << std::endl;

    std::string charset = generateCharset();
    std::cout << "Loading font from file..." << std::endl;
    bool fontLoaded = m_font->loadFromFile("C:/Windows/Fonts/arial.ttf", 24.0f, 512, 512, charset);
    if (!fontLoaded) {
        std::cerr << "Font loading failed" << std::endl;
    }
    else {
        std::cout << "Font loaded" << std::endl;
    }

    std::cout << "Creating root panel..." << std::endl;
    m_rootPanel = std::make_shared<UIPanel>();
    std::cout << "Root panel created" << std::endl;

    m_rootPanel->SetColor(Color(0.4f, 1.0f, 1.0f, 0.0f));
    m_rootPanel->setSize(SCR_WIDTH, SCR_HEIGHT);
    m_rootPanel->setPosition(0, 0);

    m_initialized = true;
    std::cout << "UIManager::init() finished" << std::endl;
}

void UIManager::render() {
    if (!m_initialized) return;
    m_renderer->beginFrame(m_rootPanel->getWidth(), m_rootPanel->getHeight());
    m_rootPanel->render(*m_renderer, m_font.get());
    m_renderer->endFrame();
}

void UIManager::onResize(int width, int height) {
    if (m_rootPanel) {
        m_rootPanel->setSize(width, height);
    }
}

void UIManager::onMouseMove(float x, float y) {
    if (m_rootPanel) {
        m_rootPanel->onMouseMove(x, y);
    }
}

void UIManager::onMouseButton(float x, float y, int button, bool pressed) {
    if (m_rootPanel) {
        m_rootPanel->onMouseButton(x, y, button, pressed);
    }
}