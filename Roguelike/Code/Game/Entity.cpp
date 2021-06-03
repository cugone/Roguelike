#include "Game/Entity.hpp"

#include "Engine/Core/StringUtils.hpp"

#include "Engine/Renderer/AnimatedSprite.hpp"

#include "Game/Actor.hpp"
#include "Game/Feature.hpp"
#include "Game/EntityDefinition.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Map.hpp"
#include "Game/Item.hpp"

Entity::~Entity() {
    /* DO NOTHING */
}

Entity::Entity()
{}

Entity::Entity(const XMLElement& elem) noexcept
{
    LoadFromXml(elem);
}

Entity::Entity(EntityDefinition* definition) noexcept
    : sprite(definition->GetSprite())
    , inventory(definition->inventory)
    , stats(definition->GetBaseStats())
{
    /* DO NOTHING */
}

void Entity::BeginFrame() {
    /* DO NOTHING */
}

void Entity::Update(TimeUtils::FPSeconds deltaSeconds) {
    sprite->Update(deltaSeconds);
}

void Entity::AppendToMesh(const Entity* const entity) noexcept {
    entity->AppendToMesh();
}

void Entity::AppendToMesh() const noexcept {
    AddVertsForCapeEquipment();
    layer->AppendToMesh(this);
    AddVertsForEquipment();
}

void Entity::CalculateLightValue() noexcept {
    SetLightValue(_self_illumination);
}

uint32_t Entity::GetLightValue() const noexcept {
    return _light_value;
}

void Entity::SetLightValue(uint32_t value) noexcept {
    _light_value = std::clamp(value, uint32_t{min_light_value}, uint32_t{max_light_value});
}

void Entity::EndFrame() {
    /* DO NOTHING */
}

Stats Entity::GetStatModifiers() const noexcept {
    return stat_modifiers;
}

const Stats& Entity::GetBaseStats() const noexcept {
    return stats;
}

Stats& Entity::GetBaseStats() noexcept {
    return stats;
}

void Entity::LoadFromXml(const XMLElement& elem) {
    DataUtils::ValidateXmlElement(elem, "entity", "definition", "name", "selflight");
    name = DataUtils::ParseXmlAttribute(elem, "name", name);
    auto xml_definition = elem.FirstChildElement("definition");
    auto definition_name = ParseEntityDefinitionName(*xml_definition);
    auto* def = EntityDefinition::GetEntityDefinitionByName(definition_name);
    sprite = def->GetSprite();
    stats = def->GetBaseStats();
}

std::string Entity::ParseEntityDefinitionName(const XMLElement& xml_definition) const {
    return StringUtils::Join(std::vector<std::string>{
        DataUtils::ParseXmlAttribute(xml_definition, "species", std::string{})
            , DataUtils::ParseXmlAttribute(xml_definition, "subspecies", std::string{})
            , DataUtils::ParseXmlAttribute(xml_definition, "sex", std::string{})
    }, '.', false);
}

void Entity::AddVertsForCapeEquipment() const noexcept {
    if(auto actor = dynamic_cast<const Actor*>(this)) {
        for(const auto& e : actor->GetEquipment()) {
            if(e && e->GetEquipSlot() == EquipSlot::Cape) {

                if(const auto* s = e->GetSprite(); !s || actor->IsInvisible()) {
                    continue;
                }
                e->AddVerts(Vector2{_position}, layer);
            }
        }
    }
}

void Entity::AddVertsForEquipment() const noexcept {
    if(auto actor = dynamic_cast<const Actor*>(this)) {
        for(const auto& e : actor->GetEquipment()) {
            if(e && e->GetEquipSlot() != EquipSlot::Cape) {

                if(const auto* s = e->GetSprite(); !s || actor->IsInvisible()) {
                    continue;
                }
                e->AddVerts(Vector2{_position}, layer);
            }
        }
    }
}

void Entity::Fight(Entity& attacker, Entity& defender) {
    attacker.OnFight.Trigger(attacker, defender);
}

void Entity::ResolveAttack(Entity& /*attacker*/, Entity& /*defender*/) {
    /* DO NOTHING */
}

bool Entity::IsVisible() const {
    return !IsNotVisible();
}

bool Entity::IsNotVisible() const {
    return !IsInvisible();
}

bool Entity::IsInvisible() const {
    if(auto* def = EntityDefinition::GetEntityDefinitionByName(name)) {
        return def->is_invisible;
    }
    return false;
}

void Entity::SetPosition(const IntVector2& position) {
    _position = position;
    _screen_position = map->WorldCoordsToScreenCoords(Vector2(_position));
}

const IntVector2& Entity::GetPosition() const {
    return _position;
}

const Vector2& Entity::GetScreenPosition() const {
    return _screen_position;
}

Stats Entity::GetStats() const {
    return GetBaseStats() + stat_modifiers;
}

void Entity::AdjustBaseStats(Stats adjustments) {
    GetBaseStats() += adjustments;
}

void Entity::AdjustStatModifiers(Stats adjustments) {
    stat_modifiers += adjustments;
}

Faction Entity::GetFaction() const noexcept {
    return _faction;
}

void Entity::SetFaction(const Faction& faction) noexcept {
    _faction = faction;
}

Faction Entity::JoinFaction(const Faction& faction) noexcept {
    SetFaction(faction);
    return _faction;
}

Rgba Entity::GetFactionAsColor() const noexcept {
    switch(_faction) {
    case Faction::None: return Rgba::Gray;
    case Faction::Player: return Rgba::Green;
    case Faction::Enemy: return Rgba::Red;
    case Faction::Neutral: return Rgba::Blue;
    default: return Rgba::Pink;
    }
}
