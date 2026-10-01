#pragma once

#include "UIXmlDocument.h"
#include <sol/sol.hpp>
#include <memory>
#include <string>
#include <unordered_map>

namespace Lindo::Graphics::UI {
    class UIWidget;
    class UIPanel;
}

namespace Lindo::Scripting {

    class UIStyleSheet;

    class LuaUIXmlProcessor {
    public:
        static void build(const UIXmlElement& document,
            const std::shared_ptr<Graphics::UI::UIPanel>& root,
            const sol::table& handlers,
            const UIStyleSheet& styles,
            std::unordered_map<std::string, std::shared_ptr<Graphics::UI::UIWidget>>& namedWidgets);
    };

}