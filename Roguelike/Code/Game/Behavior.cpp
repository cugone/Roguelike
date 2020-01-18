#include "Game/Behavior.hpp"

#include "Engine/Core/StringUtils.hpp"

#include "Game/WanderBehavior.hpp"

Behavior::Behavior(Actor* target)
: _target(target)
{
    /* DO NOTHING */
}

void Behavior::SetTarget(Actor* target) noexcept {
    _target = target;
}

Actor* Behavior::GetTarget() const noexcept {
    return _target;
}

std::shared_ptr<Behavior> Behavior::Create(const XMLElement& element) noexcept {
    DataUtils::ValidateXmlElement(element, "behavior", "", "name");
    const std::string name = DataUtils::ParseXmlAttribute(element, "name", "");
    return Behavior::Create(name);
}

std::shared_ptr<Behavior> Behavior::Create(std::string name) noexcept {
    name = StringUtils::ToLowerCase(name);
    if(name == "wander") {
        return std::make_shared<WanderBehavior>();
    } else if(name == "flee") {
        return {};
        //return std::make_shared<FleeBehavior>();
    } else if(name == "pursue") {
        return {};
        //return std::make_shared<PersueBehavior>();
    } else {
        return std::make_shared<WanderBehavior>();
    }
}
