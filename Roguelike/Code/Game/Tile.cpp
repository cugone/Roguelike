#include "Game/Tile.hpp"

#include "Engine/Core/BuildConfig.hpp"

#include "Engine/Renderer/AnimatedSprite.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IRendererService.hpp"

#include "Game/Actor.hpp"
#include "Game/Feature.hpp"
#include "Game/Layer.hpp"
#include "Game/Map.hpp"
#include "Game/TileDefinition.hpp"

#include "Game/Game.hpp"
#include "Game/GameCommon.hpp"

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

void Tile::ClearCanSee() noexcept {
    _flags_coords_lightvalue &= ~tile_flags_can_see_mask;
}

void Tile::SetCanSee() noexcept {
    _flags_coords_lightvalue &= ~tile_flags_can_see_mask;
    _flags_coords_lightvalue |= tile_flags_can_see_mask;
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
        if(HasInventory() && inventory->size() == 1) {
            inventory->GetItem(0)->GetSprite()->Update(deltaSeconds);
        }
    }
}

void Tile::DebugRender() const {
#ifdef UI_DEBUG
    Entity* entity = (actor ? dynamic_cast<Entity*>(actor) : (feature ? dynamic_cast<Entity*>(feature) : nullptr));
    if(GetGameAs<Game>()->_debug_show_all_entities && entity) {
        auto tile_bounds = GetBounds();
        g_theRenderer->SetMaterial(g_theRenderer->GetMaterial("__2D"));
        g_theRenderer->SetModelMatrix(Matrix4::I);
        g_theRenderer->DrawAABB2(tile_bounds, Rgba::Red, Rgba::NoAlpha, Vector2::One * 0.0625f);
    }
#endif
}

void Tile::ChangeTypeFromName(const std::string& name) {
    if(_type == name) {
        return;
    }
    auto* def = TileDefinition::GetTileDefinitionByName(name);
    if(!def) {
        return;
    }
    _flags_coords_lightvalue &= ~tile_flags_opaque_solid_mask;
    _flags_coords_lightvalue |= def->GetLightingBits();
    _type = name;
    layer->DirtyMesh();
}

void Tile::ChangeTypeFromGlyph(char glyph) {
    if(const auto* my_def = TileDefinition::GetTileDefinitionByName(_type); my_def && my_def->glyph == glyph) {
        return;
    }
    if(const auto* new_def = TileDefinition::GetTileDefinitionByGlyph(glyph)) {
        _type = new_def->name;
        _flags_coords_lightvalue &= ~tile_flags_opaque_solid_mask;
        _flags_coords_lightvalue |= new_def->GetLightingBits();
        layer->DirtyMesh();
    }
}

void Tile::ChangeTypeFromId(std::size_t id) {
    if(const auto* my_def = TileDefinition::GetTileDefinitionByName(_type); my_def && my_def->GetIndex() == id) {
        return;
    }
    if(const auto* new_def = TileDefinition::GetTileDefinitionByIndex(id)) {
        _type = new_def->name;
        _flags_coords_lightvalue &= ~tile_flags_opaque_solid_mask;
        _flags_coords_lightvalue |= new_def->GetLightingBits();
        layer->DirtyMesh();
    }
}

AABB2 Tile::GetBounds() const {
    return {Vector2(GetCoords()), Vector2(GetCoords() + IntVector2::One)};
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

bool Tile::CanSee() const noexcept {
    return (_flags_coords_lightvalue & tile_flags_can_see_mask) == tile_flags_can_see_mask;
}

bool Tile::IsLightDirty() const {
    return (_flags_coords_lightvalue & tile_flags_dirty_light_mask) == tile_flags_dirty_light_mask;
}

bool Tile::IsOpaque() const {
    const auto my_opaque = (_flags_coords_lightvalue & tile_flags_opaque_mask) == tile_flags_opaque_mask;
    return my_opaque || (feature && feature->IsOpaque());
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

void Tile::SetCoords(std::size_t index) {
    const auto x = static_cast<int>(index % layer->tileDimensions.x);
    const auto y = static_cast<int>(index / layer->tileDimensions.x);
    SetCoords(x, y);
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
    DirtyLight();
    layer->DirtyMesh();
}

void Tile::DecrementLightValue(int value /*= 1*/) noexcept {
    int lv = GetLightValue();
    lv = std::clamp(lv - value, min_light_value, max_light_value);
    SetLightValue(lv);
    DirtyLight();
    layer->DirtyMesh();

void Tile::ClearSky() noexcept {
    _flags_coords_lightvalue &= ~tile_flags_sky_mask;
}

void Tile::SetSky() noexcept {
    _flags_coords_lightvalue &= ~tile_flags_sky_mask;
    _flags_coords_lightvalue |= tile_flags_sky_mask;
}

bool Tile::IsSky() noexcept {
    return (_flags_coords_lightvalue & tile_flags_sky_mask) == tile_flags_sky_mask;
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

std::optional<std::vector<Tile*>> Tile::GetNeighbors(const IntVector2& direction) const {
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

std::optional<std::vector<Tile*>> Tile::GetNorthNeighbors() const {
    return GetNeighbors(IntVector2{0,-1});
}

std::optional<std::vector<Tile*>> Tile::GetNorthEastNeighbors() const {
    return GetNeighbors(IntVector2{1,-1});
}

std::optional<std::vector<Tile*>> Tile::GetEastNeighbors() const {
    return GetNeighbors(IntVector2{1,0});
}

std::optional<std::vector<Tile*>> Tile::GetSouthEastNeighbors() const {
    return GetNeighbors(IntVector2{1,1});
}

std::optional<std::vector<Tile*>> Tile::GetSouthNeighbors() const {
    return GetNeighbors(IntVector2{0,1});
}

std::optional<std::vector<Tile*>> Tile::GetSouthWestNeighbors() const {
    return GetNeighbors(IntVector2{-1,1});
}

std::optional<std::vector<Tile*>> Tile::GetWestNeighbors() const {
    return GetNeighbors(IntVector2{-1,0});
}

std::optional<std::vector<Tile*>> Tile::GetNorthWestNeighbors() const {
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
    if(layer == nullptr) {
        return;
    }
    if(auto* tile = layer->GetTile(index)) {
        tile->ClearLightDirty();
    }
}

void TileInfo::SetLightDirty() noexcept {
    if(layer == nullptr) {
        return;
    }
    if(auto* tile = layer->GetTile(index)) {
        tile->SetLightDirty();
    }
}

bool TileInfo::IsOpaque() const noexcept {
    if(layer == nullptr) {
        return false;
    }
    auto* tile = layer->GetTile(index);
    return tile && tile->IsOpaque();
}

void TileInfo::ClearOpaque() noexcept {
    if(layer) {
        return;
    }
    if(auto* tile = layer->GetTile(index)) {
        tile->ClearOpaque();
    }
}

void TileInfo::SetOpaque() noexcept {
    if(layer) {
        return;
    }
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
    if(layer == nullptr) {
        return;
    }
    if(auto* tile = layer->GetTile(index)) {
        tile->ClearSolid();
    }
}

void TileInfo::SetSolid() noexcept {
    if(layer == nullptr) {
        return;
    }
    if(auto* tile = layer->GetTile(index)) {
        tile->SetSolid();
    }
}

bool TileInfo::CanSee() const noexcept {
    if(layer == nullptr) {
        return false;
    }
    auto* tile = layer->GetTile(index);
    return tile && tile->CanSee();
}

void TileInfo::ClearCanSee() noexcept {
    if(layer == nullptr) {
        return;
    }
    if(auto* tile = layer->GetTile(index)) {
        tile->ClearCanSee();
    }
}

void TileInfo::SetCanSee() noexcept {
    if(layer == nullptr) {
        return;
    }
    if(auto* tile = layer->GetTile(index)) {
        tile->SetCanSee();
    }
}

bool TileInfo::MoveEast() noexcept {
    if(layer == nullptr) {
        return false;
    }
    if(const auto* tile = layer->GetTile(index + 1); tile != nullptr) {
        index = tile->GetIndexFromCoords();
        return true;
    }
    return false;
}

bool TileInfo::MoveWest() noexcept {
    if(layer == nullptr) {
        return false;
    }
    if(const auto* tile = layer->GetTile(index - 1); tile != nullptr) {
        index = tile->GetIndexFromCoords();
        return true;
    }
    return false;
}

bool TileInfo::MoveNorth() noexcept {
    if(layer == nullptr) {
        return false;
    }
    if(const auto* tile = layer->GetTile(index - layer->tileDimensions.x); tile != nullptr) {
        index = tile->GetIndexFromCoords();
        return true;
    }
    return false;
}

bool TileInfo::MoveSouth() noexcept {
    if(layer == nullptr) {
        return false;
    }
    if(const auto* tile = layer->GetTile(index + layer->tileDimensions.x); tile != nullptr) {
        index = tile->GetIndexFromCoords();
        return true;
    }
    return false;
}

bool TileInfo::MoveNorthWest() noexcept {
    if(layer == nullptr) {
        return false;
    }
    if(const auto* tile = layer->GetTile(index - layer->tileDimensions.x - 1); tile != nullptr) {
        index = tile->GetIndexFromCoords();
        return true;
    }
    return false;
}

bool TileInfo::MoveNorthEast() noexcept {
    if(layer == nullptr) {
        return false;
    }
    if(const auto* tile = layer->GetTile(index - layer->tileDimensions.x + 1); tile != nullptr) {
        index = tile->GetIndexFromCoords();
        return true;
    }
    return false;
}

bool TileInfo::MoveSouthWest() noexcept {
    if(layer == nullptr) {
        return false;
    }
    if(const auto* tile = layer->GetTile(index + layer->tileDimensions.x - 1); tile != nullptr) {
        index = tile->GetIndexFromCoords();
        return true;
    }
    return false;
}

bool TileInfo::MoveSouthEast() noexcept {
    if(layer == nullptr) {
        return false;
    }
    if(const auto* tile = layer->GetTile(index + layer->tileDimensions.x + 1); tile != nullptr) {
        index = tile->GetIndexFromCoords();
        return true;
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

std::array<TileInfo, 4> TileInfo::GetCardinalNeighbors() const noexcept {
    return {GetNorthNeighbor(), GetEastNeighbor(), GetSouthNeighbor(), GetWestNeighbor()};
}

TileInfo TileInfo::GetNorthWestNeighbor() const noexcept {
    TileInfo copy(*this);
    copy.MoveNorthWest();
    return copy;
}

TileInfo TileInfo::GetNorthEastNeighbor() const noexcept {
    TileInfo copy(*this);
    copy.MoveNorthEast();
    return copy;
}

TileInfo TileInfo::GetSouthEastNeighbor() const noexcept {
    TileInfo copy(*this);
    copy.MoveSouthEast();
    return copy;
}

TileInfo TileInfo::GetSouthWestNeighbor() const noexcept {
    TileInfo copy(*this);
    copy.MoveSouthWest();
    return copy;
}

std::array<TileInfo, 4> TileInfo::GetOrdinalNeighbors() const noexcept {
    return {GetNorthWestNeighbor(), GetNorthEastNeighbor(), GetSouthEastNeighbor(), GetSouthWestNeighbor()};
}

std::array<TileInfo, 8> TileInfo::GetAllNeighbors() const noexcept {
    const auto& c = GetCardinalNeighbors();
    const auto& o = GetOrdinalNeighbors();
    auto r = std::array<TileInfo, 8>{};
    for(auto i = std::size_t{0u}, j = std::size_t{0u}; i != std::size_t{4u} && j != std::size_t{8u}; (i += 1), (j += 2)) {
        r[j] = o[i];
        r[j + 1] = c[i];
    }
    return r;
}

uint32_t TileInfo::GetActorLightValue() const noexcept {
    if(HasActor()) {
        return layer->GetTile(index)->actor->GetLightValue();
    }
    return uint32_t{0u};
}

uint32_t TileInfo::GetFeatureLightValue() const noexcept {
    if(HasFeature()) {
        return layer->GetTile(index)->feature->GetLightValue();
    }
    return uint32_t{0u};
}

uint32_t TileInfo::GetLightValue() const noexcept {
    if(layer == nullptr) {
        return uint32_t{0u};
    }
    if(auto* tile = layer->GetTile(index); tile != nullptr) {
        return tile->GetLightValue();
    } else {
        return uint32_t{0u};
    }
}

void TileInfo::SetLightValue(uint32_t newValue) noexcept {
    if(layer == nullptr) {
        return;
    }
    if(auto* t = layer->GetTile(index); t != nullptr) {
        t->SetLightValue(newValue);
    }
}

uint32_t TileInfo::GetSelfIlluminationValue() const noexcept {
    if(layer == nullptr) {
        return uint32_t{0u};
    }
    if(auto* tile = layer->GetTile(index); tile != nullptr) {
        if(const auto* def = TileDefinition::GetTileDefinitionByName(tile->GetType()); def != nullptr) {
            return def->light;
        }
    }
    return uint32_t{0u};
}

uint32_t TileInfo::GetMaxLightValueFromNeighbors() const noexcept {
    if(layer == nullptr) {
        return 0;
    }
    const auto max_light = [this]()->uint32_t {
        auto idealLighting = uint32_t{0u};
        if(const auto e = GetEastNeighbor(); !e.IsOpaque()) {
            idealLighting = (std::max)(idealLighting, e.GetLightValue());
        }
        if(const auto w = GetWestNeighbor(); !w.IsOpaque()) {
            idealLighting = (std::max)(idealLighting, w.GetLightValue());
        }
        if(const auto n = GetNorthNeighbor(); !n.IsOpaque()) {
            idealLighting = (std::max)(idealLighting, n.GetLightValue());
        }
        if(const auto s = GetSouthNeighbor(); !s.IsOpaque()) {
            idealLighting = (std::max)(idealLighting, s.GetLightValue());
        }
        return idealLighting;
    }(); //IIIL
    return max_light;
}

bool TileInfo::HasActor() const noexcept {
    if(layer == nullptr) {
        return false;
    }
    if(auto* tile = layer->GetTile(index); tile != nullptr) {
        return tile->actor != nullptr;
    }
    return false;
}

bool TileInfo::HasFeature() const noexcept {
    if(layer == nullptr) {
        return false;
    }
    if(auto* tile = layer->GetTile(index); tile != nullptr) {
        return tile->feature != nullptr;
    }
    return false;
}

bool TileInfo::HasInventory() const noexcept {
    if(layer == nullptr) {
        return false;
    }
    if(auto* tile = layer->GetTile(index); tile != nullptr) {
        return tile->HasInventory();
    }
    return false;
}

bool TileInfo::IsSky() const noexcept {
    if(layer == nullptr) {
        return false;
    }
    if(auto* tile = layer->GetTile(index); tile != nullptr) {
        return tile->GetType() == "void";
    }
    return false;
}

void TileInfo::ClearSky() noexcept {
    if (layer == nullptr) {
        return;
    }
    if (auto* tile = layer->GetTile(index); tile != nullptr) {
        tile->ClearSky();
    }
}

void TileInfo::SetSky() noexcept {
    if (layer == nullptr) {
        return;
    }
    if (auto* tile = layer->GetTile(index); tile != nullptr) {
        tile->SetSky();
    }
}

bool TileInfo::IsAtEdge() const noexcept {
    if(layer == nullptr) {
        return false;
    }
    const auto value = [this]() {
        const auto e = TileInfo{*this}.MoveEast();
        const auto w = TileInfo{*this}.MoveWest();
        const auto n = TileInfo{*this}.MoveNorth();
        const auto s = TileInfo{*this}.MoveSouth();
        return !(e && w && n && s);
    }();
    return value;
}
