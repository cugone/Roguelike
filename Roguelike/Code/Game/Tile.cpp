#include "Game/Tile.hpp"

#include "Engine/Core/BuildConfig.hpp"

#include "Engine/Renderer/AnimatedSprite.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"

#include "Game/Entity.hpp"
#include "Game/Layer.hpp"
#include "Game/Map.hpp"
#include "Game/TileDefinition.hpp"

Tile::Tile()
    : _def(TileDefinition::GetTileDefinitionByName("void"))
{
    /* DO NOTHING */
}

void Tile::Update(TimeUtils::FPSeconds deltaSeconds) {
    _def->GetSprite()->Update(deltaSeconds);
    if(entity) {
        entity->Update(deltaSeconds);
    }
}

void Tile::Render(std::vector<Vertex3D>& verts, std::vector<unsigned int>& ibo, const Rgba& layer_color, size_t layer_index) const {
    if(IsInvisible()) {
        return;
    }
    AddVertsForTile(verts, ibo, layer_color, layer_index);
    if(entity) {
        entity->Render(verts, ibo, layer_color, layer_index);
    }
}

void Tile::AddVertsForTile(std::vector<Vertex3D>& verts, std::vector<unsigned int>& ibo, const Rgba& layer_color, size_t layer_index) const {
    const auto& sprite = _def->GetSprite();
    const auto& coords = sprite->GetCurrentTexCoords();

    const auto vert_left = _tile_coords.x + 0.0f;
    const auto vert_right = _tile_coords.x + 1.0f;
    const auto vert_top = _tile_coords.y + 0.0f;
    const auto vert_bottom = _tile_coords.y + 1.0f;

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
}

void Tile::ChangeTypeFromName(const std::string& name) {
    if(_def && _def->name == name) {
        return;
    }
    if(auto new_def = TileDefinition::GetTileDefinitionByName(name)) {
        _def = new_def;
    }
}

void Tile::ChangeTypeFromGlyph(char glyph) {
    if(_def && _def->glyph == glyph) {
        return;
    }
    if(auto new_def = TileDefinition::GetTileDefinitionByGlyph(glyph)) {
        _def = new_def;
    }
}

AABB2 Tile::GetBounds() const {
    return { Vector2(_tile_coords), Vector2(_tile_coords + IntVector2::ONE) };
}

const TileDefinition* Tile::GetDefinition() const {
    return _def;
}

TileDefinition* Tile::GetDefinition() {
    return const_cast<TileDefinition*>(static_cast<const Tile&>(*this).GetDefinition());
}

bool Tile::IsVisible() const {
    return _def->is_visible;
}

bool Tile::IsNotVisible() const {
    return !IsVisible();
}

bool Tile::IsInvisible() const {
    return IsNotVisible();
}

bool Tile::IsOpaque() const {
    return _def->is_opaque || color.a == 0;
}

bool Tile::IsTransparent() const {
    return !IsOpaque();
}

bool Tile::IsSolid() const {
    return _def->is_solid;
}

bool Tile::IsPassable() const {
    return !IsSolid() && !entity;
}

void Tile::SetCoords(int x, int y) {
    SetCoords(IntVector2{ x, y });
}

void Tile::SetCoords(const IntVector2& coords) {
    _tile_coords = coords;
}

const IntVector2& Tile::GetCoords() const {
    return _tile_coords;
}

Tile* Tile::GetNeighbor(const IntVector3& directionAndLayerOffset) {
    auto my_index = GetCoords();
    Map* my_map = nullptr;
    if(layer) {
        if(layer->z_index <= 0) {
            if(directionAndLayerOffset.z < 0) {
                return nullptr;
            }
        }
        if(layer->z_index >= 8) {
            if(directionAndLayerOffset.z > 0) {
                return nullptr;
            }
        }
        my_map = layer->GetMap();
    }
    if(my_map) {
        auto target_location = IntVector3{ my_index, layer->z_index };
        auto map_dims = my_map->CalcMaxDimensions();
        if(my_index.x && my_index.x < map_dims.x) {
            target_location.x += directionAndLayerOffset.x;
        } else { //x is either 0 or max
            if((my_index.x == 0 && directionAndLayerOffset.x < 0) || (my_index.x == map_dims.x && directionAndLayerOffset.x > 0)) {
                return nullptr;
            }
            target_location.x += directionAndLayerOffset.x;
        }
        if(my_index.y && my_index.y < map_dims.y) {
            target_location.y += directionAndLayerOffset.y;
        } else { //y is either 0 or max
            if((my_index.y == 0 && directionAndLayerOffset.y < 0) || (my_index.y == map_dims.y && directionAndLayerOffset.y > 0)) {
                return nullptr;
            }
            target_location.y += directionAndLayerOffset.y;
        }
        if(layer->z_index && layer->z_index < 8) {
            target_location.z += directionAndLayerOffset.z;
        } else { //z is either 0 or max
            if((layer->z_index == 0 && directionAndLayerOffset.z < 0) || (layer->z_index == 8 && directionAndLayerOffset.z > 0)) {
                return nullptr;
            }
            target_location.z += directionAndLayerOffset.z;
        }
        return my_map->GetTile(target_location);
    }
    return nullptr;
}

Tile* Tile::GetNorthNeighbor() {
    return GetNeighbor(IntVector3{0,-1,0});
}

Tile* Tile::GetNorthEastNeighbor() {
    return GetNeighbor(IntVector3{ 1,-1,0 });
}

Tile* Tile::GetEastNeighbor() {
    return GetNeighbor(IntVector3{ 1,0,0 });
}

Tile* Tile::GetSouthEastNeighbor() {
    return GetNeighbor(IntVector3{ 1,1,0 });
}

Tile* Tile::GetSouthNeighbor() {
    return GetNeighbor(IntVector3{ 0,1,0 });
}

Tile* Tile::GetSouthWestNeighbor() {
    return GetNeighbor(IntVector3{ -1,1,0 });
}

Tile* Tile::GetWestNeighbor() {
    return GetNeighbor(IntVector3{ -1,0,0 });
}

Tile* Tile::GetNorthWestNeighbor() {
    return GetNeighbor(IntVector3{ -1,-1,0 });
}

std::vector<Tile*> Tile::GetNeighbors(const IntVector2& direction) {
    auto my_index = GetCoords();
    Map* my_map = nullptr;
    if(layer) {
        my_map = layer->GetMap();
    }
    if(my_map) {
        auto target_location = my_index;
        auto map_dims = my_map->CalcMaxDimensions();
        if(my_index.x && my_index.x < map_dims.x) {
            target_location.x += direction.x;
        } else { //x is either 0 or max
            if((my_index.x == 0 && direction.x < 0) || (my_index.x == map_dims.x && direction.x > 0)) {
                return{};
            }
            target_location.x += direction.x;
        }
        if(my_index.y && my_index.y < map_dims.y) {
            target_location.y += direction.y;
        } else { //y is either 0 or max
            if((my_index.y == 0 && direction.y < 0) || (my_index.y == map_dims.y && direction.y > 0)) {
                return{};
            }
            target_location.y += direction.y;
        }
        return my_map->GetTiles(target_location);
    }
    return {};
}

std::vector<Tile*> Tile::GetNorthNeighbors() {
    return GetNeighbors(IntVector2{ 0,-1 });
}

std::vector<Tile*> Tile::GetNorthEastNeighbors() {
    return GetNeighbors(IntVector2{ 1,-1 });
}

std::vector<Tile*> Tile::GetEastNeighbors() {
    return GetNeighbors(IntVector2{ 1,0 });
}

std::vector<Tile*> Tile::GetSouthEastNeighbors() {
    return GetNeighbors(IntVector2{ 1,1 });
}

std::vector<Tile*> Tile::GetSouthNeighbors() {
    return GetNeighbors(IntVector2{ 0,1 });
}

std::vector<Tile*> Tile::GetSouthWestNeighbors() {
    return GetNeighbors(IntVector2{ -1,1 });
}

std::vector<Tile*> Tile::GetWestNeighbors() {
    return GetNeighbors(IntVector2{ -1,0 });
}

std::vector<Tile*> Tile::GetNorthWestNeighbors() {
    return GetNeighbors(IntVector2{ -1,-1 });
}
