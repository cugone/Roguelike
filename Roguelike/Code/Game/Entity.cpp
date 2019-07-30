#include "Game/Entity.hpp"

#include "Engine/Core/StringUtils.hpp"

#include "Engine/Renderer/AnimatedSprite.hpp"

#include "Game/Actor.hpp"
#include "Game/EntityDefinition.hpp"
#include "Game/Equipment.hpp"
#include "Game/Map.hpp"

Entity::Entity(const XMLElement& elem) noexcept
{
    LoadFromXml(elem);
}

Entity::Entity(EntityDefinition* definition) noexcept
    : def(definition)
{
    /* DO NOTHING */
}

void Entity::BeginFrame() {
    /* DO NOTHING */
}

void Entity::Update(TimeUtils::FPSeconds deltaSeconds) {
    sprite->Update(deltaSeconds);
}

void Entity::Render(std::vector<Vertex3D>& verts, std::vector<unsigned int>& ibo, const Rgba& layer_color, size_t layer_index) const {
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

    const float z = static_cast<float>(layer_index);
    verts.push_back(Vertex3D(Vector3(vert_bl, z), layer_color != color && color != Rgba::White ? color : layer_color, tx_bl));
    verts.push_back(Vertex3D(Vector3(vert_tl, z), layer_color != color && color != Rgba::White ? color : layer_color, tx_tl));
    verts.push_back(Vertex3D(Vector3(vert_tr, z), layer_color != color && color != Rgba::White ? color : layer_color, tx_tr));
    verts.push_back(Vertex3D(Vector3(vert_br, z), layer_color != color && color != Rgba::White ? color : layer_color, tx_br));

    const auto v_s = verts.size();
    ibo.push_back(static_cast<unsigned int>(v_s) - 4u);
    ibo.push_back(static_cast<unsigned int>(v_s) - 3u);
    ibo.push_back(static_cast<unsigned int>(v_s) - 2u);
    ibo.push_back(static_cast<unsigned int>(v_s) - 4u);
    ibo.push_back(static_cast<unsigned int>(v_s) - 2u);
    ibo.push_back(static_cast<unsigned int>(v_s) - 1u);

    AddVertsForEquipment(verts, ibo, layer_color, layer_index);
    
}

void Entity::EndFrame() {
    /* DO NOTHING */
}

const Stats& Entity::GetStatModifiers() const {
    return stat_modifiers;
}

const Stats& Entity::GetBaseStats() const {
    return def->GetBaseStats();
}

Stats& Entity::GetBaseStats() {
    return def->GetBaseStats();
}

void Entity::LoadFromXml(const XMLElement& elem) {
    DataUtils::ValidateXmlElement(elem, "entity", "definition", "name");
    name = DataUtils::ParseXmlAttribute(elem, "name", name);
    auto xml_definition = elem.FirstChildElement("definition");
    auto definition_name = ParseEntityDefinitionName(*xml_definition);
    def = EntityDefinition::GetEntityDefinitionByName(definition_name);
    sprite = def->GetSprite();
}

std::string Entity::ParseEntityDefinitionName(const XMLElement& xml_definition) const {
    return StringUtils::Join(std::vector<std::string>{
        DataUtils::ParseXmlAttribute(xml_definition, "species", std::string{})
            , DataUtils::ParseXmlAttribute(xml_definition, "subspecies", std::string{})
            , DataUtils::ParseXmlAttribute(xml_definition, "sex", std::string{})
    }, '.', false);
}

void Entity::AddVertsForEquipment(std::vector<Vertex3D>& verts, std::vector<unsigned int>& ibo, const Rgba& layer_color, size_t layer_index) const {
    if(equipment) {
        equipment->Render(verts, ibo, layer_color, layer_index);
    }
}

long long Entity::Fight(Entity& attacker, Entity& defender) {
    auto aStats = attacker.GetStats();
    auto dStats = defender.GetStats();
    const auto aAtt = aStats.GetStat(StatsID::Attack);
    const auto aSpd = aStats.GetStat(StatsID::Speed);
    const auto dDef = dStats.GetStat(StatsID::Defense);
    const auto dEva = dStats.GetStat(StatsID::Evasion);
    if(aSpd < dEva) {
        return -1; //Miss
    }
    if(aAtt < dDef) {
        return 0; //0 Dmg
    }
    auto result = aAtt - dDef;
    auto new_health = dStats.AdjustStat(StatsID::Health, -result);
    if(!new_health) {
        auto& map = defender.map;
        map->KillEntity(defender);
    }
    return result;
}

void Entity::Equip(Equipment* equipment_to_equip) {
    equipment_to_equip->owner = this;
    equipment_to_equip->ApplyStatModifer();
}

void Entity::UnEquip(Equipment* equipment_to_unequip) {
    equipment_to_unequip->RemoveStatModifer();
    equipment_to_unequip->owner = nullptr;
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
    auto cur_tile = map->GetTile(_position.x, _position.y, layer->z_index);
    cur_tile->entity = nullptr;
    _position = position;
    auto next_tile = map->GetTile(_position.x, _position.y, layer->z_index);
    next_tile->entity = this;
    tile = next_tile;
}

const IntVector2& Entity::GetPosition() const {
    return _position;
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

void Entity::UpdateAI(TimeUtils::FPSeconds /*deltaSeconds*/) {
    /* DO NOTHING */
}

