#include "UIStyleSheet.h"

#include <fstream>
#include <iterator>
#include <sstream>

namespace Lindo::Scripting {

    bool UIStyleSheet::load(const std::string& path, std::string& error) {
        std::ifstream file(path);
        if (!file) {
            error = "Cannot open style sheet: " + path;
            return false;
        }

        const std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        UIXmlDocument document;
        if (!document.parse(source, error)) {
            error = path + ": " + error;
            return false;
        }

        const UIXmlElement& root = document.root();
        for (const UIXmlElement& child : root.children) {
            if (child.name != "Style") continue;
            const std::string name = child.attribute("name");
            if (name.empty()) continue;
            auto& values = m_styles[name];
            for (const auto& attribute : child.attributes) {
                if (attribute.first != "name") values[attribute.first] = attribute.second;
            }
        }
        return true;
    }

    std::unordered_map<std::string, std::string> UIStyleSheet::resolve(const UIXmlElement& element) const {
        std::unordered_map<std::string, std::string> resolved;
        std::string styleNames = element.attribute("style");
        for (char& ch : styleNames) if (ch == ',') ch = ' ';
        std::istringstream names(styleNames);
        std::string name;
        while (names >> name) {
            const auto style = m_styles.find(name);
            if (style != m_styles.end()) {
                for (const auto& attribute : style->second) resolved[attribute.first] = attribute.second;
            }
        }
        for (const auto& attribute : element.attributes) {
            if (attribute.first != "style") resolved[attribute.first] = attribute.second;
        }
        return resolved;
    }

}