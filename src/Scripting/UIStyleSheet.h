#pragma once

#include "UIXmlDocument.h"

#include <string>
#include <unordered_map>

namespace Lindo::Scripting {

    class UIStyleSheet {
    public:
        bool load(const std::string& path, std::string& error);
        std::unordered_map<std::string, std::string> resolve(const UIXmlElement& element) const;

    private:
        std::unordered_map<std::string, std::unordered_map<std::string, std::string>> m_styles;
    };

}