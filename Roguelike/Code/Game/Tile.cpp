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

Tile::Tile()
    : _def(TileDefinition::GetTileDefinitionByName("void"))
{
    /* DO NOTHING */
}

void Tile::Update(TimeUtils::FPSeconds deltaSeconds) {
    _def->GetSprite()->Update(deltaSeconds);
    if(actor) {
        actor->Update(deltaSeconds);
    } else if(feature) {
        feature->Update(deltaSeconds);
    }
}

void Tile::Render(std::vector<Vertex3D>& verts, std::vector<unsigned int>& ibo, const Rgba& layer_color, size_t layer_index) const {
    if(IsInvisible()) {
        return;
    }
    AddVertsForTile(verts, ibo, layer_color, layer_index);
    if(actor) {
        actor->Render(verts, ibo, layer_color, layer_index);
    } else if(feature) {
        feature->Render(verts, ibo, layer_color, layer_index);
    }
    if(!canSee && haveSeen) {
        AddVertsForOverlay(verts, ibo, layer_color, layer_index);
    }
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

void Tile::AddVertsForTile(std::vector<Vertex3D>& verts, std::vector<unsigned int>& ibo, const Rgba& layer_color, size_t layer_index) const {
    if(const auto* upNeighbor = GetUpNeighbor()) {
        if(upNeighbor->IsOpaque()) {
            return;
        }
    }
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


void Tile::AddVertsForOverlay(std::vector<Vertex3D>& verts, std::vector<unsigned int>& ibo, const Rgba& layer_color, size_t layer_index) const {
    const auto coords = GetCoordsForOverlay("blue");

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

AABB2 Tile::GetCoordsForOverlay(std::string overlayName) const {
    const auto def = TileDefinition::GetTileDefinitionByName(overlayName);
    const auto sprite = def->GetSprite();
    return sprite->GetCurrentTexCoords();
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
    return _def;
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
    return !IsSolid() && !actor && (!feature || !feature->IsSolid());
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

Tile* Tile::GetNeighbor(const IntVector3& directionAndLayerOffset) const {
    if(const auto* my_map = [=]()->const Map* {
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
        const auto map_dims = IntVector3(my_map->CalcMaxDimensions(), my_map->max_layers - 1);
        const bool is_x_not_valid = (my_index.x == 0 && directionAndLayerOffset.x < 0) || (my_index.x == map_dims.x && directionAndLayerOffset.x > 0);
        const bool is_y_not_valid = (my_index.y == 0 && directionAndLayerOffset.y < 0) || (my_index.y == map_dims.y && directionAndLayerOffset.y > 0);
        const bool is_z_not_valid = (my_index.z == 0 && directionAndLayerOffset.z < 0) || (my_index.z == map_dims.z && directionAndLayerOffset.z > 0);
        const bool is_not_valid = is_x_not_valid && is_y_not_valid && is_z_not_valid;
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
    return GetNeighbor(IntVector3{ -1,-1,0 });
}

Tile* Tile::GetUpNeighbor() const {
    return GetNeighbor(IntVector3{0,0,1});
}

Tile* Tile::GetDownNeighbor() const {
    return GetNeighbor(IntVector3{0,0,-1});
}

std::vector<Tile*> Tile::GetNeighbors(const IntVector2& direction) const {
    if(const auto* my_map = [=]()->const Map* { return (layer ? layer->GetMap() : nullptr); }()) { //IIIL
        const auto my_index = GetCoords();
        const auto map_dims = my_map->CalcMaxDimensions();
        const bool is_x_not_valid = (my_index.x == 0 && direction.x < 0) || (my_index.x == map_dims.x && direction.x > 0);
        const bool is_y_not_valid = (my_index.y == 0 && direction.y < 0) || (my_index.y == map_dims.y && direction.y > 0);
        const bool is_not_valid = is_x_not_valid && is_y_not_valid;
        if(is_not_valid) {
            return {};
        }
        const auto target_location = [result = my_index, direction]() mutable -> IntVector2 { result += direction; return result; }(); //IIIL
        return my_map->GetTiles(target_location);
    }
    return {};
}

std::vector<Tile*> Tile::GetNorthNeighbors() const {
    return GetNeighbors(IntVector2{ 0,-1 });
}

std::vector<Tile*> Tile::GetNorthEastNeighbors() const {
    return GetNeighbors(IntVector2{ 1,-1 });
}

std::vector<Tile*> Tile::GetEastNeighbors() const {
    return GetNeighbors(IntVector2{ 1,0 });
}

std::vector<Tile*> Tile::GetSouthEastNeighbors() const {
    return GetNeighbors(IntVector2{ 1,1 });
}

std::vector<Tile*> Tile::GetSouthNeighbors() const {
    return GetNeighbors(IntVector2{ 0,1 });
}

std::vector<Tile*> Tile::GetSouthWestNeighbors() const {
    return GetNeighbors(IntVector2{ -1,1 });
}

std::vector<Tile*> Tile::GetWestNeighbors() const {
    return GetNeighbors(IntVector2{ -1,0 });
}

std::vector<Tile*> Tile::GetNorthWestNeighbors() const {
    return GetNeighbors(IntVector2{ -1,-1 });
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
    if(auto* asActor = dynamic_cast<Actor*>(e)) {
        actor = asActor;
    }
    if(auto* asFeature = dynamic_cast<Feature*>(e)) {
        feature = asFeature;
    }
}
