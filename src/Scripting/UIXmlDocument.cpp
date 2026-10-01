#include "UIXmlDocument.h"

#include <pugixml.hpp>

#include <functional>
#include <utility>

namespace Lindo::Scripting {
    std::string UIXmlElement::attribute(const std::string& name, const std::string& fallback) const {
        const auto it = attributes.find(name);
        return it == attributes.end() ? fallback : it->second;
    }

    bool UIXmlDocument::parse(const std::string& source, std::string& error) {
        m_root = {};
        error.clear();
        pugi::xml_document document;
        const pugi::xml_parse_result result = document.load_string(source.c_str());
        if (!result) {
            error = std::string(result.description()) + " at byte " + std::to_string(result.offset);
            return false;
        }

        const pugi::xml_node root = document.document_element();
        if (!root) {
            error = "Document has no root element";
            return false;
        }

        const std::function<void(const pugi::xml_node&, UIXmlElement&)> copyElement =
            [&copyElement](const pugi::xml_node& sourceNode, UIXmlElement& destination) {
                destination.name = sourceNode.name();
                for (const pugi::xml_attribute& attribute : sourceNode.attributes()) {
                    destination.attributes[attribute.name()] = attribute.value();
                }
                for (const pugi::xml_node& child : sourceNode.children()) {
                    if (child.type() != pugi::node_element) continue;
                    destination.children.emplace_back();
                    copyElement(child, destination.children.back());
                }
            };
        copyElement(root, m_root);
        return true;
    }
}