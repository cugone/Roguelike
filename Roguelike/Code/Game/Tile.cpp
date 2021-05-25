#include "Game/Tile.hpp"

#include "Engine/Core/BuildConfig.hpp"

#include "Engine/Renderer/AnimatedSprite.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"

#include "Game/Actor.hpp"
#include "Game/Feature.hpp"
#include "Game/Layer.hpp"
#include "Game/Map.hpp"
#include "Game/TileDefinition.hpp"

#include "Game/GameCommon.hpp"

void Tile::AddVerts() const noexcept {
    AddVertsForTile();
    if(feature) {
        feature->AddVerts();
    }
    if(HasInventory() && !inventory->empty()) {
        inventory->AddVerts(Vector2{GetCoords()}, layer);
    }
    if(actor) {
        actor->AddVerts();
    }
    if(!canSee && haveSeen) {
        //AddVertsForOverlay();
    }
}

void Tile::ClearLightDirty() noexcept {
    _flags_coords_lightvalue &= ~tile_flags_dirty_light_mask;
}

void Tile::SetLightDirty() noexcept {
    _flags_coords_lightvalue &= ~tile_flags_dirty_light_mask;
    _flags_coords_lightvalue |= tile_flags_dirty_light_mask;
}

void Tile::DirtyLight() noexcept {
    auto ti = TileInfo{layer, GetIndexFromCoords()};
    if(ti.IsLightDirty()) {
        return;
    }
    layer->GetMap()->DirtyTileLight(ti);
}

void Tile::ClearOpaque() noexcept {
    _flags_coords_lightvalue &= ~tile_flags_opaque_mask;
}

void Tile::SetOpaque() noexcept {
    _flags_coords_lightvalue &= ~tile_flags_opaque_mask;
    _flags_coords_lightvalue |= tile_flags_opaque_mask;
}

void Tile::ClearSolid() noexcept {
    _flags_coords_lightvalue &= ~tile_flags_solid_mask;
}

void Tile::SetSolid() noexcept {
    _flags_coords_lightvalue &= ~tile_flags_solid_mask;
    _flags_coords_lightvalue |= tile_flags_solid_mask;
}

void Tile::Update(TimeUtils::FPSeconds deltaSeconds) {
    if(auto* def = TileDefinition::GetTileDefinitionByName(_type)) {
        def->GetSprite()->Update(deltaSeconds);
        if(feature) {
            feature->Update(deltaSeconds);
        }
        if(actor) {
            actor->Update(deltaSeconds);
        }
    }
}

void Tile::AddVertsForTile() const noexcept {
    if(IsInvisible()) {
        return;
    }
    const auto* def = TileDefinition::GetTileDefinitionByName(_type);
    const auto& sprite = def->GetSprite();
    const auto& coords = sprite->GetCurrentTexCoords();

    const auto tile_coords = GetCoords();
    const auto vert_left = tile_coords.x + 0.0f;
    const auto vert_right = tile_coords.x + 1.0f;
    const auto vert_top = tile_coords.y + 0.0f;
    const auto vert_bottom = tile_coords.y + 1.0f;

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
        clr.ScaleRGB(MathUtils::RangeMap(static_cast<float>(GetLightValue()), static_cast<float>(min_light_value), static_cast<float>(max_light_value), min_light_scale, max_light_scale));
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

void Tile::DebugRender(Renderer& renderer) const {
    Entity* entity = (actor ? dynamic_cast<Entity*>(actor) : (feature ? dynamic_cast<Entity*>(feature) : nullptr));
    if(g_theGame->_show_all_entities && entity) {
        auto tile_bounds = GetBounds();
        renderer.SetMaterial(renderer.GetMaterial("__2D"));
        renderer.SetModelMatrix(Matrix4::I);
        renderer.DrawAABB2(tile_bounds, Rgba::Red, Rgba::NoAlpha, Vector2::ONE * 0.0625f);
    }
}

void Tile::AddVertsForOverlay() const noexcept {
    const auto overlayName = std::string{"blue"};
    const auto coords = GetCoordsForOverlay(overlayName);

    const auto tile_coords = GetCoords();
    const auto vert_left = tile_coords.x + 0.0f;
    const auto vert_right = tile_coords.x + 1.0f;
    const auto vert_top = tile_coords.y + 0.0f;
    const auto vert_bottom = tile_coords.y + 1.0f;

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

    const auto newColor = layer_color != color && color != Rgba::White ? color : layer_color;
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

    auto* sprite = GetSpriteForOverlay(overlayName);
    builder.End(sprite->GetMaterial());

}

AABB2 Tile::GetCoordsForOverlay(std::string overlayName) const {
    const auto def = TileDefinition::GetTileDefinitionByName(overlayName);
    const auto sprite = def->GetSprite();
    return sprite->GetCurrentTexCoords();
}

AnimatedSprite* Tile::GetSpriteForOverlay(std::string overlayName) const {
    const auto def = TileDefinition::GetTileDefinitionByName(overlayName);
    return def->GetSprite();
}

void Tile::ChangeTypeFromName(const std::string& name) {
    if(_type == name) {
        return;
    }
    auto* def = TileDefinition::GetTileDefinitionByName(name);
    if(!def) {
        return;
    }
    _flags_coords_lightvalue &= ~def->GetLightingBits();
    _flags_coords_lightvalue |= def->GetLightingBits();
    _type = name;
    layer->DirtyMesh();
}

void Tile::ChangeTypeFromGlyph(char glyph) {
    if(const auto* my_def = TileDefinition::GetTileDefinitionByName(_type); my_def && my_def->glyph == glyph) {
        return;
    }
    if(auto* new_def = TileDefinition::GetTileDefinitionByGlyph(glyph)) {
        _type = new_def->name;
        _flags_coords_lightvalue &= ~new_def->GetLightingBits();
        _flags_coords_lightvalue |= new_def->GetLightingBits();
        layer->DirtyMesh();
    }
}

AABB2 Tile::GetBounds() const {
    return {Vector2(GetCoords()), Vector2(GetCoords() + IntVector2::ONE)};
}

bool Tile::IsVisible() const {
    if(const auto* def = TileDefinition::GetTileDefinitionByName(_type)) {
        return def->is_visible;
    }
    return false;
}

bool Tile::IsNotVisible() const {
    return !IsVisible();
}

bool Tile::IsInvisible() const {
    return IsNotVisible();
}

bool Tile::IsLightDirty() const {
    return (_flags_coords_lightvalue & tile_flags_dirty_light_mask) == tile_flags_dirty_light_mask;
}

bool Tile::IsOpaque() const {
    return (_flags_coords_lightvalue & tile_flags_opaque_mask) == tile_flags_opaque_mask;
}

bool Tile::IsTransparent() const {
    return !IsOpaque();
}

bool Tile::IsSolid() const {
    const auto my_solid = (_flags_coords_lightvalue & tile_flags_solid_mask) == tile_flags_solid_mask;
    return my_solid || actor || (feature && feature->IsSolid());
}

bool Tile::IsPassable() const {
    return !IsSolid();
}

void Tile::SetEntrance() noexcept {
    if(auto* def = TileDefinition::GetTileDefinitionByName(_type)) {
        def->is_entrance = true;
    }
}

void Tile::SetExit() noexcept {
    if(auto* def = TileDefinition::GetTileDefinitionByName(_type)) {
        def->is_exit = true;
    }
}

void Tile::ClearEntrance() noexcept {
    if(auto* def = TileDefinition::GetTileDefinitionByName(_type)) {
        def->is_entrance = false;
    }
}

void Tile::ClearExit() noexcept {
    if(auto* def = TileDefinition::GetTileDefinitionByName(_type)) {
        def->is_exit = false;
    }
}

bool Tile::IsEntrance() const {
    if(auto* def = TileDefinition::GetTileDefinitionByName(_type)) {
        return def->is_entrance;
    }
    return false;
}

bool Tile::IsExit() const {
    if(auto* def = TileDefinition::GetTileDefinitionByName(_type)) {
        return def->is_exit;
    }
    return false;
}

bool Tile::HasInventory() const noexcept {
    return inventory != nullptr;
}

Item* Tile::AddItem(Item* item) noexcept {
    if(!inventory) {
        inventory = std::make_unique<Inventory>();
    }
    return inventory->AddItem(item);
}

Item* Tile::AddItem(const std::string& name) noexcept {
    if(!inventory) {
        inventory = std::make_unique<Inventory>();
    }
    return inventory->AddItem(name);
}

void Tile::SetCoords(int x, int y) {
    SetCoords(IntVector2{x, y});
}

void Tile::SetCoords(const IntVector2& coords) {
    _flags_coords_lightvalue |= DataUtils::ShiftLeft(coords.y, tile_y_offset) | DataUtils::ShiftLeft(coords.x, tile_x_offset);
}

const IntVector2 Tile::GetCoords() const {
    const int y = DataUtils::ShiftRight(_flags_coords_lightvalue & tile_coords_y_mask, tile_y_offset);
    const int x = DataUtils::ShiftRight(_flags_coords_lightvalue & tile_coords_x_mask, tile_x_offset);
    return IntVector2{x, y};
}

std::size_t Tile::GetIndexFromCoords() const noexcept {
    const auto coords = GetCoords();
    return static_cast<std::size_t>(coords.y) * layer->tileDimensions.x + coords.x;
}

uint32_t Tile::GetFlags() const noexcept {
    return _flags_coords_lightvalue & tile_flags_mask;
}

void Tile::SetFlags(uint32_t flags) noexcept {
    _flags_coords_lightvalue &= ~tile_flags_mask;
    _flags_coords_lightvalue |= flags;
}

void Tile::CalculateLightValue() noexcept {
    const auto max_neighbor_value = GetMaxLightValueFromNeighbors();
    const auto neighbor_value = max_neighbor_value > uint32_t{0u} ? max_neighbor_value - uint32_t{1u} : uint32_t{0u};
    const auto self_value = [this]() {
        TileInfo info{};
        info.layer = this->layer;
        info.index = this->GetIndexFromCoords();
        return info.GetSelfIlluminationValue();
    }();
    const auto feature_value = [this]() {
        if(feature) {
            feature->CalculateLightValue();
            return feature->GetLightValue();
        }
        return uint32_t{0u};
    }(); //IIIL
    const auto actor_value = [this]() {
        if(actor) {
            actor->CalculateLightValue();
            return actor->GetLightValue();
        }
        return uint32_t{0u};
    }(); //IIIL
    SetLightValue(std::clamp(g_current_global_light + neighbor_value + feature_value + actor_value, uint32_t{min_light_value}, uint32_t{max_light_value}));
}

uint32_t Tile::GetLightValue() const noexcept {
    return _flags_coords_lightvalue & tile_flags_light_mask;
}

void Tile::SetLightValue(uint32_t newValue) noexcept {
    _flags_coords_lightvalue &= ~tile_flags_light_mask;
    _flags_coords_lightvalue |= (newValue & tile_flags_light_mask);
}

void Tile::IncrementLightValue(int value /*= 1*/) noexcept {
    int lv = GetLightValue();
    lv = std::clamp(lv + value, min_light_value, max_light_value);
    SetLightValue(lv);
}

void Tile::DecrementLightValue(int value /*= 1*/) noexcept {
    int lv = GetLightValue();
    lv = std::clamp(lv - value, min_light_value, max_light_value);
    SetLightValue(lv);
}

Tile* Tile::GetNeighbor(const IntVector3& directionAndLayerOffset) const {
    if(const auto* my_map = [&]()->const Map* {
        if(layer) {
            //layer is valid but the requested index is out of bounds.
            if((layer->z_index <= 0 && directionAndLayerOffset.z < 0) || (layer->z_index >= layer->GetMap()->max_layers - 1 && directionAndLayerOffset.z > 0)) {
                return nullptr;
            }
            return layer->GetMap();
        }
        return nullptr;
        }()) { //IIIL
        const auto my_index = IntVector3(GetCoords(), layer->z_index);
        const auto map_dims = IntVector3(my_map->CalcMaxDimensions(), my_map->max_layers);
        const bool is_x_not_valid = (my_index.x == 0 && directionAndLayerOffset.x < 0) || (my_index.x == (map_dims.x - 1) && directionAndLayerOffset.x > 0);
        const bool is_y_not_valid = (my_index.y == 0 && directionAndLayerOffset.y < 0) || (my_index.y == (map_dims.y - 1) && directionAndLayerOffset.y > 0);
        const bool is_z_not_valid = (my_index.z == 0 && directionAndLayerOffset.z < 0) || (my_index.z == (map_dims.z - 1) && directionAndLayerOffset.z > 0);
        const bool is_not_valid = is_x_not_valid || is_y_not_valid || is_z_not_valid;
        if(is_not_valid) {
            return nullptr;
        }
        const auto target_location = [result = my_index, directionAndLayerOffset]() mutable -> IntVector3 { result += directionAndLayerOffset; return result; }(); //IIIL
        return my_map->GetTile(target_location);
    }
    return nullptr;
}

Tile* Tile::GetNorthNeighbor() const {
    return GetNeighbor(IntVector3{0,-1,0});
}

Tile* Tile::GetNorthEastNeighbor() const {
    return GetNeighbor(IntVector3{1,-1,0});
}

Tile* Tile::GetEastNeighbor() const {
    return GetNeighbor(IntVector3{1,0,0});
}

Tile* Tile::GetSouthEastNeighbor() const {
    return GetNeighbor(IntVector3{1,1,0});
}

Tile* Tile::GetSouthNeighbor() const {
    return GetNeighbor(IntVector3{0,1,0});
}

Tile* Tile::GetSouthWestNeighbor() const {
    return GetNeighbor(IntVector3{-1,1,0});
}

Tile* Tile::GetWestNeighbor() const {
    return GetNeighbor(IntVector3{-1,0,0});
}

Tile* Tile::GetNorthWestNeighbor() const {
    return GetNeighbor(IntVector3{-1,-1,0});
}

Tile* Tile::GetUpNeighbor() const {
    return GetNeighbor(IntVector3{0,0,1});
}

Tile* Tile::GetDownNeighbor() const {
    return GetNeighbor(IntVector3{0,0,-1});
}

uint32_t Tile::GetMaxLightValueFromNeighbors() const noexcept {
    const auto n = GetCardinalNeighbors();
    const auto pred = [](const Tile* a, const Tile* b) {
        const auto a_value = a ? a->GetLightValue() : min_light_value;
        const auto b_value = b ? b->GetLightValue() : min_light_value;
        return a_value < b_value;
    };
    const auto max_iter = std::max_element(std::cbegin(n), std::cend(n), pred);
    return (*max_iter) != nullptr ? (*max_iter)->GetLightValue() : uint32_t{0u};
}

std::vector<Tile*> Tile::GetNeighbors(const IntVector2& direction) const {
    if(const auto* my_map = [=]()->const Map* { return (layer ? layer->GetMap() : nullptr); }()) { //IIIL
        const auto& my_index = GetCoords();
        const auto map_dims = my_map->CalcMaxDimensions();
        const bool is_x_not_valid = (my_index.x == 0 && direction.x < 0) || (my_index.x == map_dims.x && direction.x > 0);
        const bool is_y_not_valid = (my_index.y == 0 && direction.y < 0) || (my_index.y == map_dims.y && direction.y > 0);
        const bool is_not_valid = is_x_not_valid || is_y_not_valid;
        if(is_not_valid) {
            return {};
        }
        const auto target_location = [result = my_index, direction]() mutable -> IntVector2 { result += direction; return result; }(); //IIIL
        return my_map->GetTiles(target_location);
    }
    return {};
}

std::array<Tile*, 8> Tile::GetNeighbors() const {
    return {GetNorthWestNeighbor(), GetNorthNeighbor(), GetNorthEastNeighbor(), GetEastNeighbor(),
             GetSouthEastNeighbor(), GetSouthNeighbor(), GetSouthWestNeighbor(), GetWestNeighbor()};
}

std::array<Tile*, 4> Tile::GetCardinalNeighbors() const {
    return {GetNorthNeighbor(), GetEastNeighbor(), GetSouthNeighbor(), GetWestNeighbor()};
}

std::array<Tile*, 4> Tile::GetOrdinalNeighbors() const {
    return {GetNorthWestNeighbor(), GetNorthEastNeighbor(), GetSouthEastNeighbor(), GetSouthWestNeighbor()};

}

std::vector<Tile*> Tile::GetNorthNeighbors() const {
    return GetNeighbors(IntVector2{0,-1});
}

std::vector<Tile*> Tile::GetNorthEastNeighbors() const {
    return GetNeighbors(IntVector2{1,-1});
}

std::vector<Tile*> Tile::GetEastNeighbors() const {
    return GetNeighbors(IntVector2{1,0});
}

std::vector<Tile*> Tile::GetSouthEastNeighbors() const {
    return GetNeighbors(IntVector2{1,1});
}

std::vector<Tile*> Tile::GetSouthNeighbors() const {
    return GetNeighbors(IntVector2{0,1});
}

std::vector<Tile*> Tile::GetSouthWestNeighbors() const {
    return GetNeighbors(IntVector2{-1,1});
}

std::vector<Tile*> Tile::GetWestNeighbors() const {
    return GetNeighbors(IntVector2{-1,0});
}

std::vector<Tile*> Tile::GetNorthWestNeighbors() const {
    return GetNeighbors(IntVector2{-1,-1});
}


Entity* Tile::GetEntity() const noexcept {
    if(actor) {
        return actor;
    }
    if(feature) {
        return feature;
    }
    return nullptr;
}

void Tile::SetEntity(Entity* e) noexcept {
    if(!e) {
        return;
    }
    if(auto* asActor = dynamic_cast<Actor*>(e)) {
        actor = asActor;
    }
    if(auto* asFeature = dynamic_cast<Feature*>(e)) {
        feature = asFeature;
    }
    layer->DirtyMesh();
}

const std::string Tile::GetType() const noexcept {
    return _type;
}

bool TileInfo::IsLightDirty() const noexcept {
    if(layer == nullptr) {
        return false;
    }
    auto* tile = layer->GetTile(index);
    return tile && tile->IsLightDirty();
}

void TileInfo::ClearLightDirty() noexcept {
    if(auto* tile = layer->GetTile(index)) {
        tile->ClearLightDirty();
    }
}

void TileInfo::SetLightDirty() noexcept {
    if(auto* tile = layer->GetTile(index)) {
        tile->SetLightDirty();
    }
}

bool TileInfo::IsOpaque() const noexcept {
    if(layer == nullptr) {
        return false;
    }
    if(auto* tile = layer->GetTile(index)) {
        return tile->IsOpaque();
    }
    return false;
}

void TileInfo::ClearOpaque() noexcept {
    if(auto* tile = layer->GetTile(index)) {
        tile->ClearOpaque();
    }
}

void TileInfo::SetOpaque() noexcept {
    if(auto* tile = layer->GetTile(index)) {
        tile->SetOpaque();
    }
}

bool TileInfo::IsSolid() const noexcept {
    if(layer == nullptr) {
        return false;
    }
    auto* tile = layer->GetTile(index);
    return tile && tile->IsSolid();
}

void TileInfo::ClearSolid() noexcept {
    auto* tile = layer->GetTile(index);
    tile->ClearSolid();
}

void TileInfo::SetSolid() noexcept {
    auto* tile = layer->GetTile(index);
    tile->SetSolid();
}

bool TileInfo::MoveEast() noexcept {
    if(layer == nullptr) {
        return false;
    }
    if(const auto* tile = layer->GetTile(index); tile != nullptr) {
        if(tile = tile->GetEastNeighbor(); tile != nullptr) {
            index = tile->GetIndexFromCoords();
            return true;
        }
    }
    return false;
}

bool TileInfo::MoveWest() noexcept {
    if(layer == nullptr) {
        return false;
    }
    if(const auto* tile = layer->GetTile(index); tile != nullptr) {
        if(tile = tile->GetWestNeighbor(); tile != nullptr) {
            index = tile->GetIndexFromCoords();
            return true;
        }
    }
    return false;
}

bool TileInfo::MoveNorth() noexcept {
    if(layer == nullptr) {
        return false;
    }
    if(const auto* tile = layer->GetTile(index); tile != nullptr) {
        if(tile = tile->GetNorthNeighbor(); tile != nullptr) {
            index = tile->GetIndexFromCoords();
            return true;
        }
    }
    return false;
}

bool TileInfo::MoveSouth() noexcept {
    if(layer == nullptr) {
        return false;
    }
    if(const auto* tile = layer->GetTile(index); tile != nullptr) {
        if(tile = tile->GetSouthNeighbor(); tile != nullptr) {
            index = tile->GetIndexFromCoords();
            return true;
        }
    }
    return false;
}

TileInfo TileInfo::GetNorthNeighbor() const noexcept {
    TileInfo copy(*this);
    copy.MoveNorth();
    return copy;
}

TileInfo TileInfo::GetSouthNeighbor() const noexcept {
    TileInfo copy(*this);
    copy.MoveSouth();
    return copy;
}

TileInfo TileInfo::GetEastNeighbor() const noexcept {
    TileInfo copy(*this);
    copy.MoveEast();
    return copy;
}

TileInfo TileInfo::GetWestNeighbor() const noexcept {
    TileInfo copy(*this);
    copy.MoveWest();
    return copy;
}

uint32_t TileInfo::GetLightValue() const noexcept {
    if(layer == nullptr) {
        return 0;
    }
    if(auto* tile = layer->GetTile(index); tile != nullptr) {
        return tile->GetLightValue();
    } else {
        return 0;
    }
}

void TileInfo::SetLightValue(uint32_t newValue) noexcept {
    if(layer == nullptr) {
        return;
    }
    auto* t = layer->GetTile(index);
    if(t->GetLightValue() == newValue) {
        return;
    }
    layer->DirtyMesh();
    t->SetLightValue(newValue);
}

uint32_t TileInfo::GetSelfIlluminationValue() const noexcept {
    if(layer == nullptr) {
        return 0;
    }
    if(auto* tile = layer->GetTile(index); tile != nullptr) {
        if(const auto* def = TileDefinition::GetTileDefinitionByName(tile->GetType()); def != nullptr) {
            return def->light;
        }
    }
    return 0;
}

uint32_t TileInfo::GetMaxLightValueFromNeighbors() const noexcept {
    if(layer == nullptr) {
        return 0;
    }
    const auto max_light = [this]()->uint32_t {
        const auto values = std::array<uint32_t, 4>{
            GetEastNeighbor().GetLightValue(), GetWestNeighbor().GetLightValue(),
            GetNorthNeighbor().GetLightValue(), GetSouthNeighbor().GetLightValue(),
        };
        return (*std::max_element(std::cbegin(values), std::cend(values)));
    }(); //IIIL
    return max_light;
}
