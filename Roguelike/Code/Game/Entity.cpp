#include "Game/Entity.hpp"

#include "Engine/Core/StringUtils.hpp"

#include "Engine/Renderer/AnimatedSprite.hpp"

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
    return base_stats;
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

bool Entity::CanMoveDiagonallyToNeighbor(const IntVector2& direction) const {
    const auto target = _position + direction;
    if(_position.x == target.x || _position.y == target.y) {
        return true;
    }
    {
        const auto test_tiles = map->GetTiles(_position.x, target.y);
        for(const auto* t : test_tiles) {
            if(t->IsSolid()) {
                return false;
            }
        }
    }
    {
        const auto test_tiles = map->GetTiles(target.x, _position.y);
        for(const auto* t : test_tiles) {
            if(t->IsSolid()) {
                return false;
            }
        }
    }
    return true;
}

void Entity::AddVertsForEquipment(std::vector<Vertex3D>& verts, std::vector<unsigned int>& ibo, const Rgba& layer_color, size_t layer_index) const {
    if(equipment) {
        equipment->Render(verts, ibo, layer_color, layer_index);
    }
}

void Entity::Fight(Entity& attacker, Entity& defender) {
    auto aStats = attacker.GetStats();
    auto dStats = defender.GetStats();

}

bool Entity::Acted() const {
    return _acted;
}

void Entity::Act() {
    Act(true);
}

void Entity::Act(bool value) {
    _acted = value;
}

void Entity::DontAct() {
    Act(false);
}

void Entity::Move(const IntVector2& direction) {
    if(CanMoveDiagonallyToNeighbor(direction)) {
        const auto target_position = _position + direction;
        auto target_tile = map->GetTile(target_position.x, target_position.y, 0);
        if(target_tile) {
            if(target_tile->IsPassable()) {
                SetPosition(_position + direction);
            } else {
                auto target_entity = target_tile->entity;
                if(target_entity) {
                    Fight(*this, *target_entity);
                }
            }
        }
    }
    Act();
}

void Entity::MoveNorth() {
    Move(IntVector2{0,-1});
}

void Entity::MoveNorthEast() {
    Move(IntVector2{1,-1});
}

void Entity::MoveEast() {
    Move(IntVector2{ 1,0 });
}

void Entity::MoveSouthEast() {
    Move(IntVector2{ 1,1 });
}

void Entity::MoveSouth() {
    Move(IntVector2{0,1});
}

void Entity::MoveSouthWest() {
    Move(IntVector2{ -1,1 });
}

void Entity::MoveWest() {
    Move(IntVector2{ -1,0 });
}

void Entity::MoveNorthWest() {
    Move(IntVector2{ -1,-1 });
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
    return base_stats + stat_modifiers;
}

void Entity::AdjustBaseStats(Stats adjustments) {
    base_stats += adjustments;
}

void Entity::AdjustStatModifiers(Stats adjustments) {
    stat_modifiers += adjustments;
}

void Entity::UpdateAI(TimeUtils::FPSeconds /*deltaSeconds*/) {
    /* DO NOTHING */
}

