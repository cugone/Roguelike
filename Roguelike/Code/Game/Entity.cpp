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

Entity::Entity() {
    /* DO NOTHING */
}

Entity::Entity(const a2de::XMLElement& elem) noexcept
{
    LoadFromXml(elem);
}

Entity::Entity(EntityDefinition* definition) noexcept
    : def(definition)
    , sprite(def->GetSprite())
    , inventory(def->inventory)
    , stats(definition->GetBaseStats())
{
    /* DO NOTHING */
}

void Entity::BeginFrame() {
    /* DO NOTHING */
}

void Entity::Update(a2de::TimeUtils::FPSeconds deltaSeconds) {
    sprite->Update(deltaSeconds);
}

void Entity::AddVerts() noexcept {
    AddVertsForCapeEquipment();
    AddVertsForSelf();
    AddVertsForEquipment();
}

void Entity::AddVertsForSelf() noexcept {
    if(!sprite || IsInvisible()) {
        return;
    }
    const auto& coords = sprite->GetCurrentTexCoords();

    const auto vert_left = _position.x + 0.0f;
    const auto vert_right = _position.x + 1.0f;
    const auto vert_top = _position.y + 0.0f;
    const auto vert_bottom = _position.y + 1.0f;

    const auto vert_bl = a2de::Vector2(vert_left, vert_bottom);
    const auto vert_tl = a2de::Vector2(vert_left, vert_top);
    const auto vert_tr = a2de::Vector2(vert_right, vert_top);
    const auto vert_br = a2de::Vector2(vert_right, vert_bottom);

    const auto tx_left = coords.mins.x;
    const auto tx_right = coords.maxs.x;
    const auto tx_top = coords.mins.y;
    const auto tx_bottom = coords.maxs.y;

    const auto tx_bl = a2de::Vector2(tx_left, tx_bottom);
    const auto tx_tl = a2de::Vector2(tx_left, tx_top);
    const auto tx_tr = a2de::Vector2(tx_right, tx_top);
    const auto tx_br = a2de::Vector2(tx_right, tx_bottom);

    const float z = static_cast<float>(layer->z_index);
    const a2de::Rgba layer_color = layer->color;

    auto& builder = layer->GetMeshBuilder();
    const auto newColor = layer_color != color && color != a2de::Rgba::White ? color : layer_color;
    const auto normal = -a2de::Vector3::Z_AXIS;

    builder.Begin(a2de::PrimitiveType::Triangles);
    builder.SetColor(newColor);
    builder.SetNormal(normal);

    builder.SetUV(tx_bl);
    builder.AddVertex(a2de::Vector3{vert_bl, z});

    builder.SetUV(tx_tl);
    builder.AddVertex(a2de::Vector3{vert_tl, z});

    builder.SetUV(tx_tr);
    builder.AddVertex(a2de::Vector3{vert_tr, z});

    builder.SetUV(tx_br);
    builder.AddVertex(a2de::Vector3{vert_br, z});

    builder.AddIndicies(a2de::Mesh::Builder::Primitive::Quad);
    builder.End(sprite->GetMaterial());

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

void Entity::LoadFromXml(const a2de::XMLElement& elem) {
    a2de::DataUtils::ValidateXmlElement(elem, "entity", "definition", "name");
    name = a2de::DataUtils::ParseXmlAttribute(elem, "name", name);
    auto xml_definition = elem.FirstChildElement("definition");
    auto definition_name = ParseEntityDefinitionName(*xml_definition);
    def = EntityDefinition::GetEntityDefinitionByName(definition_name);
    sprite = def->GetSprite();
    stats = def->GetBaseStats();
}

std::string Entity::ParseEntityDefinitionName(const a2de::XMLElement& xml_definition) const {
    return a2de::StringUtils::Join(std::vector<std::string>{
        a2de::DataUtils::ParseXmlAttribute(xml_definition, "species", std::string{})
            , a2de::DataUtils::ParseXmlAttribute(xml_definition, "subspecies", std::string{})
            , a2de::DataUtils::ParseXmlAttribute(xml_definition, "sex", std::string{})
    }, '.', false);
}

void Entity::AddVertsForCapeEquipment() const {
    if(auto actor = dynamic_cast<const Actor*>(this)) {
        for(const auto& e : actor->GetEquipment()) {
            if(e && e->GetEquipSlot() == EquipSlot::Cape) {

                if(const auto* s = e->GetSprite(); !s || actor->IsInvisible()) {
                    continue;
                }
                e->AddVerts(a2de::Vector2{_position}, layer);
            }
        }
    }
}

void Entity::AddVertsForEquipment() const {
    if(auto actor = dynamic_cast<const Actor*>(this)) {
        for(const auto& e : actor->GetEquipment()) {
            if(e && e->GetEquipSlot() != EquipSlot::Cape) {

                if(const auto* s = e->GetSprite(); !s || actor->IsInvisible()) {
                    continue;
                }
                e->AddVerts(a2de::Vector2{_position}, layer);
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
    return def->is_invisible;
}

void Entity::SetPosition(const a2de::IntVector2& position) {
    _position = position;
    _screen_position = map->WorldCoordsToScreenCoords(a2de::Vector2(_position));
}

const a2de::IntVector2& Entity::GetPosition() const {
    return _position;
}

const a2de::Vector2& Entity::GetScreenPosition() const {
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
