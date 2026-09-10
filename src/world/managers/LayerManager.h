#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cstdint>
#include <stdexcept>

namespace Lindo::World {

    using LayerMask = uint32_t;

    /**
     * @brief Единый менеджер и глобальная реестр-система Слоев (Layers) и Тегов (Tags).
     */
    class LayerManager {
    public:
        static constexpr uint8_t MAX_LAYERS = 32;
        static constexpr LayerMask LAYER_EVERYTHING = 0xFFFFFFFF;
        static constexpr LayerMask LAYER_NOTHING    = 0x0;

        static LayerManager& get() {
            static LayerManager instance;
            return instance;
        }

        // --- ЛОГИКА СЛОЕВ (LAYERS) ---

        /**
         * @brief Регистрирует имя слоя с указанным индексом (0..31).
         */
        void nameLayer(uint8_t layerIndex, const std::string& name) {
            if (layerIndex >= MAX_LAYERS) {
                throw std::out_of_range("[LayerManager] Layer index must be between 0 and 31");
            }
            m_layerToName[layerIndex] = name;
            m_nameToLayer[name] = layerIndex;
        }

        /**
         * @brief Возвращает индекс слоя по имени.
         */
        uint8_t getLayerByName(const std::string& name) const {
            auto it = m_nameToLayer.find(name);
            if (it != m_nameToLayer.end()) return it->second;
            return 0; // По умолчанию Layer 0 ("Default")
        }

        /**
         * @brief Возвращает имя слоя по индексу.
         */
        std::string getLayerName(uint8_t layerIndex) const {
            auto it = m_layerToName.find(layerIndex);
            if (it != m_layerToName.end()) return it->second;
            return "Undefined";
        }

        /**
         * @brief Получает битовую маску из списка имён слоёв.
         */
        LayerMask getMask(const std::vector<std::string>& layerNames) const {
            LayerMask mask = 0;
            for (const auto& name : layerNames) {
                mask |= (1 << getLayerByName(name));
            }
            return mask;
        }

        // --- ЛОГИКА ТЕГОВ (TAGS) ---

        /**
         * @brief Регистрирует новый тег в единую систему.
         */
        void registerTag(const std::string& tag) {
            if (m_registeredTags.find(tag) == m_registeredTags.end()) {
                m_registeredTags.insert(tag);
            }
        }

        /**
         * @brief Проверяет валидность существования тега в системе.
         */
        bool isTagRegistered(const std::string& tag) const {
            return m_registeredTags.find(tag) != m_registeredTags.end();
        }

    private:
        LayerManager() {
            // Базовая инициализация слоев по умолчанию
            nameLayer(0, "Default");
            nameLayer(1, "TransparentFX");
            nameLayer(2, "Ignore Raycast");
            nameLayer(3, "Water");
            nameLayer(4, "UI");

            // Базовые теги
            registerTag("Untagged");
            registerTag("Player");
            registerTag("MainCamera");
            registerTag("Enemy");
        }

        std::unordered_map<uint8_t, std::string> m_layerToName;
        std::unordered_map<std::string, uint8_t> m_nameToLayer;
        std::unordered_set<std::string> m_registeredTags;
    };
}