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
: _self_illumination{max_light_value}
{}

Entity::Entity(const XMLElement& elem) noexcept
: _self_illumination{max_light_value}
{
    LoadFromXml(elem);
}

Entity::Entity(EntityDefinition* definition) noexcept
    : def(definition)
    , sprite(def->GetSprite())
    , inventory(def->inventory)
    , stats(definition->GetBaseStats())
    , _self_illumination{max_light_value}
{
    /* DO NOTHING */
}

void Entity::BeginFrame() {
    /* DO NOTHING */
}

void Entity::Update(TimeUtils::FPSeconds deltaSeconds) {
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

    const auto vert_bl = Vector2(vert_left, vert_bottom);
    const auto vert_tl = Vector2(vert_left, vert_top);
    const auto vert_tr = Vector2(vert_right, vert_top);
    const auto vert_br = Vector2(vert_right, vert_bottom);

    const auto tx_left = coords.mins.x;
    const auto tx_right = coords.maxs.x;
    const auto tx_top = coords.mins.y;
    const auto tx_bottom = coords.maxs.y;

    const auto tx_bl = Vector2(tx_left, tx_bottom);
    const auto tx_tl = Vector2(tx_left, tx_top);
    const auto tx_tr = Vector2(tx_right, tx_top);
    const auto tx_br = Vector2(tx_right, tx_bottom);

    const float z = static_cast<float>(layer->z_index);
    const Rgba layer_color = layer->color;

    auto& builder = layer->GetMeshBuilder();
    const auto newColor = [&]() {
        auto clr = layer_color != color && color != Rgba::White ? color : layer_color;
        clr.ScaleRGB(GetLightValue() / static_cast<float>(max_light_value));
        return clr;
    }(); //IIIL
    const auto normal = -Vector3::Z_AXIS;

    builder.Begin(PrimitiveType::Triangles);
    builder.SetColor(newColor);
    builder.SetNormal(normal);

    builder.SetUV(tx_bl);
    builder.AddVertex(Vector3{vert_bl, z});

    builder.SetUV(tx_tl);
    builder.AddVertex(Vector3{vert_tl, z});

    builder.SetUV(tx_tr);
    builder.AddVertex(Vector3{vert_tr, z});

    builder.SetUV(tx_br);
    builder.AddVertex(Vector3{vert_br, z});

    builder.AddIndicies(Mesh::Builder::Primitive::Quad);
    builder.End(sprite->GetMaterial());

}

void Entity::CalculateLightValue() noexcept {
    SetLightValue(_self_illumination);
}

uint32_t Entity::GetLightValue() const noexcept {
    return _light_value;
}

void Entity::SetLightValue(uint32_t value) noexcept {
    _light_value = value;
    _light_value = std::clamp(_light_value + _self_illumination, uint32_t{0}, uint32_t{15});
    layer->DirtyMesh();
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
    DataUtils::ValidateXmlElement(elem, "entity", "definition", "name");
    name = DataUtils::ParseXmlAttribute(elem, "name", name);
    auto xml_definition = elem.FirstChildElement("definition");
    auto definition_name = ParseEntityDefinitionName(*xml_definition);
    def = EntityDefinition::GetEntityDefinitionByName(definition_name);
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

void Entity::AddVertsForCapeEquipment() const {
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

void Entity::AddVertsForEquipment() const {
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
    return def->is_invisible;
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
