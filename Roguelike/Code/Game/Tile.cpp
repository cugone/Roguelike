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

void Tile::AddVerts() const noexcept {
    AddVertsForTile();
    //if(actor) {
    //    actor->AddVerts();
    //} else if(feature) {
    //    feature->AddVerts();
    //} else if(HasInventory() && !inventory->empty()) {
    //    inventory->AddVerts(Vector2{_tile_coords}, layer);
    //}
    //if(!canSee && haveSeen) {
    //    AddVertsForOverlay();
    //}

    //if(g_theGame->current_cursor; g_theGame->current_cursor->GetCoords() == _tile_coords) {
    //    auto& builder = layer->GetMeshBuilder();
    //    g_theGame->current_cursor->AddVertsForCursor(builder);
    //}
}

void Tile::Update(TimeUtils::FPSeconds deltaSeconds) {
    _def->GetSprite()->Update(deltaSeconds);
}

void Tile::AddVertsForTile() const noexcept {
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

    const float z = static_cast<float>(layer->z_index);
    const Rgba layer_color = layer->color;
    auto& builder = layer->GetMeshBuilder();

    const auto newColor = layer_color != color && color != Rgba::White ? color : layer_color;
    const auto normal = -Vector3::Z_AXIS;

    //builder.Begin(PrimitiveType::Triangles);
    //builder.SetColor(newColor);
    //builder.SetNormal(normal);

    //builder.SetUV(tx_bl);
    //builder.AddVertex(Vector3{vert_bl, z});
    //
    //builder.SetUV(tx_tl);
    //builder.AddVertex(Vector3{vert_tl, z});

    //builder.SetUV(tx_tr);
    //builder.AddVertex(Vector3{vert_tr, z});

    //builder.SetUV(tx_br);
    //builder.AddVertex(Vector3{vert_br, z});

    //builder.AddIndicies(Mesh::Builder::Primitive::Quad);

    //builder.End(sprite->GetMaterial());

    builder.Begin(PrimitiveType::Points);
    builder.SetColor(newColor);

    builder.AddVertex(Vector3{vert_bl, z});

    builder.AddIndicies(Mesh::Builder::Primitive::Point);

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
    return _def->is_opaque || (feature && feature->IsOpaque());
}

bool Tile::IsTransparent() const {
    return _def->is_transparent || (feature && feature->IsTransparent());
}

bool Tile::IsSolid() const {
    return _def->is_solid || actor || (feature && feature->IsSolid());
}

bool Tile::IsPassable() const {
    return !IsSolid();
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
    SetCoords(IntVector2{ x, y });
}

void Tile::SetCoords(const IntVector2& coords) {
    _tile_coords = coords;
}

const IntVector2& Tile::GetCoords() const {
    return _tile_coords;
}

int Tile::GetIndexFromCoords() const noexcept {
    return _tile_coords.y * layer->tileDimensions.x + _tile_coords.x;
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
    return { GetNorthWestNeighbor(), GetNorthNeighbor(), GetNorthEastNeighbor(), GetEastNeighbor(),
             GetSouthEastNeighbor(), GetSouthNeighbor(), GetSouthWestNeighbor(), GetWestNeighbor() };
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
