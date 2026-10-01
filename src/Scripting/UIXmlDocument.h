#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace Lindo::Scripting {

    struct UIXmlElement {
        std::string name;
        std::unordered_map<std::string, std::string> attributes;
        std::vector<UIXmlElement> children;

        std::string attribute(const std::string& name, const std::string& fallback = {}) const;
    };

    class UIXmlDocument {
    public:
        bool parse(const std::string& source, std::string& error);
        const UIXmlElement& root() const { return m_root; }

    private:
        UIXmlElement m_root;
    };

}