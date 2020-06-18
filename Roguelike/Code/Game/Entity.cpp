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

Entity::Entity(const XMLElement& elem) noexcept
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
    //auto& vbo = layer->GetVbo();
    auto& builder = layer->GetMeshBuilder();
    const auto newColor = layer_color != color && color != Rgba::White ? color : layer_color;
    const auto normal = -Vector3::Z_AXIS;

    builder.Begin(PrimitiveType::Triangles);
    builder.SetColor(newColor);
    builder.SetNormal(normal);

    builder.SetUV(tx_tl);
    builder.AddVertex(Vector3{vert_tl, z});

    builder.SetUV(tx_bl);
    builder.AddVertex(Vector3{vert_bl, z});

    builder.SetUV(tx_br);
    builder.AddVertex(Vector3{vert_br, z});

    builder.SetUV(tx_tr);
    builder.AddVertex(Vector3{vert_tr, z});

    builder.AddIndicies(Mesh::Builder::Primitive::Quad);
    builder.End(sprite->GetMaterial());

    //vbo.push_back(Vertex3D{Vector3{vert_tl, z}, newColor, tx_tl, normal});
    //vbo.push_back(Vertex3D{Vector3{vert_bl, z}, newColor, tx_bl, normal});
    //vbo.push_back(Vertex3D{Vector3{vert_br, z}, newColor, tx_br, normal});
    //vbo.push_back(Vertex3D{Vector3{vert_tr, z}, newColor, tx_tr, normal});

    //const auto v_s = static_cast<unsigned int>(vbo.size());
    //auto& ibo = layer->GetIbo();
    //ibo.push_back(v_s - 4u);
    //ibo.push_back(v_s - 3u);
    //ibo.push_back(v_s - 2u);
    //ibo.push_back(v_s - 4u);
    //ibo.push_back(v_s - 2u);
    //ibo.push_back(v_s - 1u);
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
    defender.OnFight.Trigger(attacker, defender);
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
