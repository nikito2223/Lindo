#include "Component/GameObject/GameObject.h"
#include <iostream>
#include <Physics/RigidBody.h>

void GameObject::printHierarchy(int indent) {
    std::string ind(indent, ' ');
    std::cout << ind << "GameObject: " << name
        << " (Tag: " << tag << ")"
        << " Active: " << (isActive ? "Yes" : "No")
        << " Components: " << components.size()
        << std::endl;

    for (auto* child : children) {
        child->printHierarchy(indent + 2);
    }
}

// ----- Обновление и рендеринг -----
void GameObject::Update(float deltaTime, const glm::vec3& gravity) {
    if (!isActive) return;

    // Первый вызов — запускаем все компоненты
    if (!started) {
        for (auto& comp : components) {
            comp->OnStart();
        }
        started = true;
    }

    // Обновляем компоненты
    for (auto& comp : components) {
        comp->OnUpdate(deltaTime);
    }

    // Интеграция физики (можно вынести в отдельный шаг)
    if (auto* rb = getComponent<RigidBody>()) {
        rb->integrate(deltaTime, gravity);
    }

    // Обновляем детей
    for (auto* child : children) {
        child->Update(deltaTime, gravity);
    }
}

void GameObject::Draw(Shader& shader) {
    if (!isActive) return;
    for (auto& comp : components) {
        comp->OnDraw(shader);
    }
    for (auto* child : children) {
        child->Draw(shader);
    }
}

// ----- Очистка кэша (если компонент был удален напрямую) -----
void GameObject::invalidateCache() {
    componentCache.clear();
}



void GameObject::addChild(GameObject* child) {
    if (child && child->parent != this) {
        // Удаляем из предыдущего родителя
        if (child->parent) {
            child->parent->removeChild(child);
        }
        child->parent = this;
        children.push_back(child);
    }
}

void GameObject::removeChild(GameObject* child) {
    auto it = std::find(children.begin(), children.end(), child);
    if (it != children.end()) {
        child->parent = nullptr;
        children.erase(it);
    }
}

// Получить всех потомков (рекурсивно)
std::vector<GameObject*> GameObject::getAllChildren() {
    std::vector<GameObject*> result = children;
    for (auto* child : children) {
        auto grandChildren = child->getAllChildren();
        result.insert(result.end(), grandChildren.begin(), grandChildren.end());
    }
    return result;
}

// ----- Поиск объектов (НОВИНКА!) -----
GameObject* GameObject::findChildByName(const std::string& childName) {
    for (auto* child : children) {
        if (child->name == childName) return child;
        auto* found = child->findChildByName(childName);
        if (found) return found;
    }
    return nullptr;
}

std::vector<GameObject*> GameObject::findChildrenByTag(const std::string& childTag) {
    std::vector<GameObject*> result;
    for (auto* child : children) {
        if (child->tag == childTag) result.push_back(child);
        auto found = child->findChildrenByTag(childTag);
        result.insert(result.end(), found.begin(), found.end());
    }
    return result;
}

// ----- Управление активностью (НОВИНКА!) -----
void GameObject::setActive(bool active) {
    if (isActive == active) return;
    isActive = active;

    // Рекурсивно для всех детей
    for (auto* child : children) {
        child->setActive(active);
    }
}

// ----- Мировые координаты с учетом родителя (НОВИНКА!) -----
glm::vec3 GameObject::getWorldPosition() const {
    if (parent) {
        return parent->getWorldPosition() + transform.position;
    }
    return transform.position;
}

glm::mat4 GameObject::getWorldMatrix() const {
    if (parent) {
        return parent->getWorldMatrix() * transform.getMatrix();
    }
    return transform.getMatrix();
}