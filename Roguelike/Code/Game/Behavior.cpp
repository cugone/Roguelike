#include "Game/Behavior.hpp"

#include "Engine/Core/StringUtils.hpp"

#include "Game/WanderBehavior.hpp"
#include "Game/FleeBehavior.hpp"
#include "Game/PursueBehavior.hpp"

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

void Behavior::SetName(const std::string& name) noexcept {
    _name = StringUtils::ToLowerCase(name);
}

const std::string& Behavior::GetName() const noexcept {
    return _name;
}

std::string Behavior::NameFromId(BehaviorID id) {
    switch(id) {
    case BehaviorID::None:
        return std::string{"none"};
    case BehaviorID::Wander:
        return std::string{"wander"};
    case BehaviorID::Flee:
        return std::string{"flee"};
    case BehaviorID::Pursue:
        return std::string{"pursue"};
    case BehaviorID::Sleep:
        return std::string{"sleep"};
    default:
        return std::string{"none"};
    }
}

BehaviorID Behavior::IdFromName(std::string name) {
    name = StringUtils::ToLowerCase(name);
    if(name == "none") {
        return BehaviorID::None;
    } else if(name == "wander") {
        return BehaviorID::Wander;
    } else if(name == "flee") {
        return BehaviorID::Flee;
    } else if(name == "pursue") {
        return BehaviorID::Pursue;
    } else if(name == "sleep") {
        return BehaviorID::Sleep;
    } else {
        return BehaviorID::None;
    }
}

std::shared_ptr<Behavior> Behavior::Create(const XMLElement& element) noexcept {
    DataUtils::ValidateXmlElement(element, "behavior", "", "name");
    const std::string name = DataUtils::ParseXmlAttribute(element, "name", std::string{});
    return Behavior::Create(name);
}

std::shared_ptr<Behavior> Behavior::Create(std::string name) noexcept {
    name = StringUtils::ToLowerCase(name);
    if(name == "wander") {
        return std::make_shared<WanderBehavior>();
    } else if(name == "flee") {
        return std::make_shared<FleeBehavior>();
    } else if(name == "pursue") {
        return std::make_shared<PursueBehavior>();
    } else {
        return std::make_shared<WanderBehavior>();
    }
}
