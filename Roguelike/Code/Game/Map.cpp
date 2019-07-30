#include "Game/Map.hpp"

#include "Engine/Core/BuildConfig.hpp"
#include "Engine/Core/DataUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/StringUtils.hpp"

#include "Engine/Math/Vector4.hpp"
#include "Engine/Math/IntVector3.hpp"

#include "Engine/Renderer/Material.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"

#include "Game/Actor.hpp"
#include "Game/Entity.hpp"
#include "Game/EntityDefinition.hpp"
#include "Game/Equipment.hpp"
#include "Game/EquipmentDefinition.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Layer.hpp"
#include "Game/TileDefinition.hpp"

#include <algorithm>
#include <sstream>

unsigned long long Map::default_map_index = 0ull;

void Map::SetDebugGridColor(const Rgba& gridColor) {
    auto* layer = GetLayer(0);
    layer->debug_grid_color = gridColor;
}

AABB2 Map::CalcWorldBounds() const {
    return {Vector2::ZERO, CalcMaxDimensions()};
}

std::vector<Tile*> Map::PickTilesFromWorldCoords(const Vector2& worldCoords) const {
    auto world_bounds = CalcWorldBounds();
    if(MathUtils::IsPointInside(world_bounds, worldCoords)) {
        std::vector<Tile*> results = GetTiles(IntVector2{ worldCoords });
        return results;
    }
    return {};
}

Tile* Map::PickTileFromWorldCoords(const Vector2& worldCoords, int layerIndex) const {
    auto world_bounds = CalcWorldBounds();
    if(MathUtils::IsPointInside(world_bounds, worldCoords)) {
        return GetTile(IntVector3{ worldCoords, layerIndex });
    }
    return nullptr;
}

std::vector<Tile*> Map::PickTilesFromMouseCoords(const Vector2& mouseCoords) const {
    auto world_coords = Vector2{ g_theRenderer->ConvertScreenToWorldCoords(camera, mouseCoords) };
    return PickTilesFromWorldCoords(world_coords);
}

Tile* Map::PickTileFromMouseCoords(const Vector2& mouseCoords, int layerIndex) const {
    auto world_coords = Vector2{ g_theRenderer->ConvertScreenToWorldCoords(camera, mouseCoords) };
    return PickTileFromWorldCoords(world_coords, layerIndex);
}

bool Map::MoveOrAttack(Actor* actor, Tile* tile) {
    if(!actor || !tile) {
        return false;
    }
    if(actor->MoveTo(tile)) {
        return true;
    } else {
        Entity::Fight(*actor, *tile->entity);
        return actor->MoveTo(tile);
    }
}

Map::Map(Renderer& renderer, const XMLElement& elem) noexcept
    : _renderer(renderer)
{
    if(!LoadFromXML(elem)) {
        ERROR_AND_DIE("Could not load map.");
    }
}

Map::~Map() noexcept {
    for(auto& entity : _entities) {
        delete entity;
        entity = nullptr;
    }
    _entities.clear();
    _entities.shrink_to_fit();
}

void Map::BeginFrame() {
    for(auto& layer : _layers) {
        layer->BeginFrame();
    }
}

void Map::Update(TimeUtils::FPSeconds deltaSeconds) {
    UpdateLayers(deltaSeconds);
    UpdateEntities(deltaSeconds);
}

void Map::UpdateLayers(TimeUtils::FPSeconds deltaSeconds) {
    for(auto& layer : _layers) {
        layer->Update(deltaSeconds);
    }
}

void Map::SetPriorityLayer(std::size_t i) {
    if(i >= _layers.size()) {
        return;
    }
    BringLayerToFront(i);
}

void Map::BringLayerToFront(std::size_t i) {
    auto first = std::begin(_layers);
    auto curr = first + i;
    auto next = curr + 1;
    auto end = std::end(_layers);
    while(next != end) {
        std::iter_swap(curr, next);
        curr++;
        next = curr + 1;
    }
}

void Map::UpdateEntities(TimeUtils::FPSeconds deltaSeconds) {
    for(auto& entity : _entities) {
        entity->Update(deltaSeconds);
    }
}

void Map::UpdateEntityAI(TimeUtils::FPSeconds deltaSeconds) {
    for(auto& entity : _entities) {
        entity->UpdateAI(deltaSeconds);
    }
}

void Map::Render(Renderer& renderer) const {
    for(const auto& layer : _layers) {
        layer->Render(renderer);
    }

    if(auto* tile = PickTileFromMouseCoords(g_theInputSystem->GetMouseCoords(), 0)) {
        auto tile_bounds = tile->GetBounds();
        renderer.SetMaterial(renderer.GetMaterial("__2D"));
        renderer.SetModelMatrix(Matrix4::I);
        renderer.DrawAABB2(tile_bounds, Rgba::White, Rgba::NoAlpha, Vector2::ONE * 0.0625f);
    }
    if(g_theGame->_show_all_entities) {
        for(const auto& e : _entities) {
            auto tile_bounds = e->tile->GetBounds();
            renderer.SetMaterial(renderer.GetMaterial("__2D"));
            renderer.SetModelMatrix(Matrix4::I);
            renderer.DrawAABB2(tile_bounds, Rgba::Red, Rgba::NoAlpha, Vector2::ONE * 0.0625f);
        }
    }
}

void Map::DebugRender(Renderer& renderer) const {
    for(const auto& layer : _layers) {
        layer->DebugRender(renderer);
    }
    if(g_theGame->_show_grid) {
        if(g_theGame->_show_grid) {
            renderer.SetModelMatrix(Matrix4::I);
            const auto* layer = GetLayer(0);
            renderer.DrawWorldGrid2D(layer->tileDimensions, layer->debug_grid_color);
        }
    }
    if(g_theGame->_show_world_bounds) {
        auto bounds = CalcWorldBounds();
        renderer.SetModelMatrix(Matrix4::I);
        renderer.DrawAABB2(bounds, Rgba::Cyan, Rgba::NoAlpha);
    }
}

void Map::EndFrame() {
    for(auto& layer : _layers) {
        layer->EndFrame();
    }
}

bool Map::IsTileInView(const IntVector2& tileCoords) {
    return IsTileInView(IntVector3{tileCoords, 0});
}

bool Map::IsTileInView(const IntVector3& tileCoords) {
    return IsTileInView(GetTile(tileCoords));
}

bool Map::IsTileInView(Tile* tile) {
    if(!tile || (tile && !tile->layer)) {
        return false;
    }
    const auto tile_bounds = tile->GetBounds();
    const auto view_bounds = tile->layer->CalcViewBounds(Vector2(tile->GetCoords()));
    return MathUtils::DoAABBsOverlap(tile_bounds, view_bounds);
}

bool Map::IsEntityInView(Entity* entity) {
    return entity && IsTileInView(entity->tile);
}

void Map::FocusTileAt(const IntVector3& position) {
    if(const auto* tile = GetTile(position)) {
        camera.SetPosition(Vector3{ position });
    }
}

void Map::FocusEntity(Entity* entity) {
    if(entity) {
        FocusTileAt(IntVector3(entity->tile->GetCoords(), entity->layer->z_index));
    }
}

Vector2 Map::CalcMaxDimensions() const {
    Vector2 results{ 1.0f, 1.0f };
    for(const auto& layer : _layers) {
        const auto& cur_layer_dimensions = layer->tileDimensions;
        if(cur_layer_dimensions.x > results.x) {
            results.x = static_cast<float>(cur_layer_dimensions.x);
        }
        if(cur_layer_dimensions.y > results.y) {
            results.y = static_cast<float>(cur_layer_dimensions.y);
        }
    }
    return results;
}

float Map::CalcMaxViewHeight() const {
    auto max_view_height_elem = std::max_element(std::begin(_layers), std::end(_layers),
    [](const std::unique_ptr<Layer>& a, const std::unique_ptr<Layer>& b)->bool{
        return a->viewHeight < b->viewHeight;
    });
    return (*max_view_height_elem)->viewHeight;
}

Material* Map::GetTileMaterial() const {
    return _current_tileMaterial;
}

void Map::SetTileMaterial(Material* material) {
    _current_tileMaterial = material;
}

void Map::ResetTileMaterial() {
    _current_tileMaterial = _default_tileMaterial;
}

std::size_t Map::GetLayerCount() const {
    return _layers.size();
}

Layer* Map::GetLayer(std::size_t index) const {
    if(index >= _layers.size()) {
        return nullptr;
    }
    return _layers[index].get();
}

std::vector<Tile*> Map::GetTiles(const IntVector2& location) const {
    return GetTiles(location.x, location.y);
}

Tile* Map::GetTile(const IntVector3& locationAndLayerIndex) const {
    return GetTile(locationAndLayerIndex.x, locationAndLayerIndex.y, locationAndLayerIndex.z);
}

std::vector<Tile*> Map::GetTiles(int x, int y) const {
    std::vector<Tile*> results{};
    for(auto i = std::size_t{0}; i < GetLayerCount(); ++i) {
        if(auto* cur_layer = GetLayer(i)) {
            results.push_back(cur_layer->GetTile(x, y));
        }
    }
    return results;
}

Tile* Map::GetTile(int x, int y, int z) const {
    if(auto* layer = GetLayer(z)) {
        return layer->GetTile(x, y);
    }
    return nullptr;
}

bool Map::LoadFromXML(const XMLElement& elem) {

    DataUtils::ValidateXmlElement(elem, "map", "tiles,layers,material", "name", "entities,entityTypes,entityMap,equipment");

    LoadNameForMap(elem);
    LoadMaterialsForMap(elem);
    LoadTileDefinitionsForMap(elem);
    LoadLayersForMap(elem);
    LoadEquipmentForMap(elem);
    LoadEntitiesForMap(elem);
    LoadEntityTypesForMap(elem);
    PlaceEntitiesOnMap(elem);
    return true;
}

void Map::LoadNameForMap(const XMLElement& elem) {
    std::ostringstream ss;
    ss << "MAP " << ++default_map_index;
    std::string default_name = ss.str();
    _name = DataUtils::ParseXmlAttribute(elem, "name", default_name);
}

void Map::LoadMaterialsForMap(const XMLElement& elem) {
    if(auto xml_material = elem.FirstChildElement("material")) {
        DataUtils::ValidateXmlElement(*xml_material, "material", "", "name");
        auto src = DataUtils::ParseXmlAttribute(*xml_material, "name", std::string{ "__invalid" });
        _default_tileMaterial = g_theRenderer->GetMaterial(src);
        _current_tileMaterial = _default_tileMaterial;
    }
}

void Map::LoadLayersForMap(const XMLElement &elem) {
    if(auto xml_layers = elem.FirstChildElement("layers")) {
        DataUtils::ValidateXmlElement(*xml_layers, "layers", "layer", "");
        std::size_t layer_count = DataUtils::GetChildElementCount(*xml_layers, "layer");
        if(layer_count > _max_layers) {
            std::ostringstream ss;
            ss << "Layer count of map " << _name << " is greater than the maximum allowed (" << _max_layers << ").";
            ss << "\nOnly the first " << _max_layers << " layers will be used.";
            ss.flush();
            g_theFileLogger->LogLine(ss.str());
        }

        auto layer_index = 0;
        auto max_layers = _max_layers;
        _layers.reserve(layer_count);
        DataUtils::ForEachChildElement(*xml_layers, "layer",
            [this, &layer_index, max_layers](const XMLElement& xml_layer) {
            if(static_cast<std::size_t>(layer_index) < max_layers) {
                _layers.emplace_back(std::make_unique<Layer>(this, xml_layer));
                _layers.back()->z_index = layer_index++;
            }
        });
        _layers.shrink_to_fit();
    }
}

void Map::LoadTileDefinitionsForMap(const XMLElement& elem) {
    if(auto xml_tileset = elem.FirstChildElement("tiles")) {
        DataUtils::ValidateXmlElement(*xml_tileset, "tiles", "", "src");
        const auto src = DataUtils::ParseXmlAttribute(*xml_tileset, "src", std::string{});
        if(src.empty()) {
            ERROR_AND_DIE("Map tiles source is empty.");
        }
        LoadTileDefinitionsFromFile(src);
    }
}

void Map::LoadTileDefinitionsFromFile(const std::filesystem::path& src) {
    namespace FS = std::filesystem;
    if(!FS::exists(src)) {
        std::ostringstream ss;
        ss << "Entities file at " << src << " could not be found.";
        ERROR_AND_DIE(ss.str().c_str());
    }
    tinyxml2::XMLDocument doc;
    auto xml_result = doc.LoadFile(src.string().c_str());
    if(xml_result != tinyxml2::XML_SUCCESS) {
        std::ostringstream ss;
        ss << "Map " << _name << " failed to load. Tiles source file at " << src << " could not be loaded.";
        ERROR_AND_DIE(ss.str().c_str());
    }
    if(auto xml_root = doc.RootElement()) {
        DataUtils::ValidateXmlElement(*xml_root, "tileDefinitions", "spritesheet,tileDefinition", "");
        if(auto xml_spritesheet = xml_root->FirstChildElement("spritesheet")) {
            _tileset_sheet = g_theRenderer->CreateSpriteSheet(*xml_spritesheet);
            if(_tileset_sheet) {
                DataUtils::ForEachChildElement(*xml_root, "tileDefinition",
                    [this](const XMLElement& elem) {
                    TileDefinition::CreateTileDefinition(g_theRenderer, elem, _tileset_sheet);
                });
            }
        }
    }
}

void Map::LoadEntitiesForMap(const XMLElement& elem) {
    if(auto* xml_map_entities_source = elem.FirstChildElement("entities")) {
        DataUtils::ValidateXmlElement(*xml_map_entities_source, "entities", "", "src");
        const auto src = DataUtils::ParseXmlAttribute(*xml_map_entities_source, "src", std::string{});
        if(src.empty()) {
            ERROR_AND_DIE("Map entities source is empty. Do not define element if map does not have entities.");
        }
        LoadEntitiesFromFile(src);
    }
}

void Map::LoadEntitiesFromFile(const std::filesystem::path& src) {
    namespace FS = std::filesystem;
    if(!FS::exists(src)) {
        std::ostringstream ss;
        ss << "Entities file at " << src << " could not be found.";
        ERROR_AND_DIE(ss.str().c_str());
    }
    tinyxml2::XMLDocument doc;
    auto xml_result = doc.LoadFile(src.string().c_str());
    if(xml_result != tinyxml2::XML_SUCCESS) {
        std::ostringstream ss;
        ss << "Map " << _name << " failed to load. Entities source file at " << src << " could not be loaded.";
        ERROR_AND_DIE(ss.str().c_str());
    }
    if(auto* xml_entities_root = doc.RootElement()) {
        DataUtils::ValidateXmlElement(*xml_entities_root, "entities", "definitions,entity", "");
        if(auto* xml_definitions = xml_entities_root->FirstChildElement("definitions")) {
            DataUtils::ValidateXmlElement(*xml_definitions, "definitions", "", "src");
            const auto definitions_src = DataUtils::ParseXmlAttribute(*xml_definitions, "src", std::string{});
            if(definitions_src.empty()) {
                ERROR_AND_DIE("Entity definitions source is empty.");
            }
            FS::path def_src(definitions_src);
            if(!FS::exists(def_src)) {
                ERROR_AND_DIE("Entity definitions source not found.");
            }
            LoadEntityDefinitionsFromFile(def_src);
        }
    }
}

void Map::LoadEntityDefinitionsFromFile(const std::filesystem::path& src) {
    namespace FS = std::filesystem;
    if(!FS::exists(src)) {
        std::ostringstream ss;
        ss << "Entity Definitions file at " << src << " could not be found.";
        ERROR_AND_DIE(ss.str().c_str());
    }
    tinyxml2::XMLDocument doc;
    auto xml_result = doc.LoadFile(src.string().c_str());
    if(xml_result != tinyxml2::XML_SUCCESS) {
        std::ostringstream ss;
        ss << "Entity Definitions at " << src << " failed to load.";
        ERROR_AND_DIE(ss.str().c_str());
    }
    if(auto* xml_root = doc.RootElement()) {
        DataUtils::ValidateXmlElement(*xml_root, "entityDefinitions", "spritesheet,entityDefinition", "");
        auto* xml_spritesheet = xml_root->FirstChildElement("spritesheet");
        _entity_sheet = _renderer.CreateSpriteSheet(*xml_spritesheet);
        DataUtils::ForEachChildElement(*xml_root, "entityDefinition",
        [this](const XMLElement& elem) {
            EntityDefinition::CreateEntityDefinition(_renderer, elem, _entity_sheet);
        });
    }
}

void Map::LoadEntityTypesForMap(const XMLElement& elem) {
    if(auto* xml_entity_types = elem.FirstChildElement("entityTypes")) {
        DataUtils::ValidateXmlElement(*xml_entity_types, "entityTypes", "player", "");
        DataUtils::ForEachChildElement(*xml_entity_types, "",
            [this](const XMLElement& elem) {
            auto name = DataUtils::GetElementName(elem);
            DataUtils::ValidateXmlElement(elem, name, "", "lookAndFeel");
            const auto definitionName = DataUtils::ParseXmlAttribute(elem, "lookAndFeel", "");
            auto* definition = EntityDefinition::GetEntityDefinitionByName(definitionName);
            auto type = std::make_unique<EntityType>();
            type->name = name;
            type->definition = definition;
            _entity_types.insert(std::make_pair(name, std::move(type)));
        });
    }
}

void Map::LoadEquipmentForMap(const XMLElement& elem) {
    if(auto* xml_map_equipment_source = elem.FirstChildElement("equipment")) {
        DataUtils::ValidateXmlElement(*xml_map_equipment_source, "equipment", "", "src");
        const auto src = DataUtils::ParseXmlAttribute(*xml_map_equipment_source, "src", std::string{});
        if(src.empty()) {
            ERROR_AND_DIE("Map equipment source is empty. Do not define element if map does not have equipment.");
        }
        LoadEquipmentFromFile(src);
    }
}

void Map::LoadEquipmentFromFile(const std::filesystem::path& src) {
    namespace FS = std::filesystem;
    if(!FS::exists(src)) {
        std::ostringstream ss;
        ss << "Equipment file at " << src << " could not be found.";
        ERROR_AND_DIE(ss.str().c_str());
    }
    tinyxml2::XMLDocument doc;
    auto xml_result = doc.LoadFile(src.string().c_str());
    if(xml_result != tinyxml2::XML_SUCCESS) {
        std::ostringstream ss;
        ss << "Map " << _name << " failed to load. Equipment source file at " << src << " could not be loaded.";
        ERROR_AND_DIE(ss.str().c_str());
    }
    if(auto* xml_equipment_root = doc.RootElement()) {
        DataUtils::ValidateXmlElement(*xml_equipment_root, "equipments", "definitions,equipment", "");
        auto* xml_definitions = xml_equipment_root->FirstChildElement("definitions");
        
        DataUtils::ValidateXmlElement(*xml_definitions, "definitions", "", "src");
        const auto definitions_src = DataUtils::ParseXmlAttribute(*xml_definitions, "src", std::string{});
        if(definitions_src.empty()) {
            ERROR_AND_DIE("Equipment definitions source is empty.");
        }
        FS::path def_src(definitions_src);
        if(!FS::exists(def_src)) {
            ERROR_AND_DIE("Equipment definitions source not found.");
        }
        LoadEquipmentDefinitionsFromFile(def_src);
        LoadEquipmentTypesForMap(*xml_equipment_root);
    }
}

void Map::LoadEquipmentTypesForMap(const XMLElement& elem) {
    if (auto* xml_entity_types = elem.FirstChildElement("entityTypes")) {
        DataUtils::ValidateXmlElement(*xml_entity_types, "entityTypes", "player", "");
        DataUtils::ForEachChildElement(*xml_entity_types, "",
            [this](const XMLElement& elem) {
            auto name = DataUtils::GetElementName(elem);
            DataUtils::ValidateXmlElement(elem, name, "", "lookAndFeel");
            const auto definitionName = DataUtils::ParseXmlAttribute(elem, "lookAndFeel", "");
            auto* definition = EntityDefinition::GetEntityDefinitionByName(definitionName);
            auto type = std::make_unique<EntityType>();
            type->name = name;
            type->definition = definition;
            _entity_types.insert(std::make_pair(name, std::move(type)));
        });
    }

    DataUtils::ForEachChildElement(elem, "equipment",
        [](const XMLElement& elem) {
        DataUtils::ValidateXmlElement(elem, "equipment", "definition", "name");
        DataUtils::ForEachChildElement(elem, "definition",
            [](const XMLElement& elem) {
            DataUtils::ValidateXmlElement(elem, "definition", "", "slot,type,subtype,color");
            EquipmentType etype{};
            const auto slot = DataUtils::ParseXmlAttribute(elem, "slot", "");
            const auto type = std::string{};//DataUtils::ParseXmlAttribute(elem, "type", "");
            const auto subtype = DataUtils::ParseXmlAttribute(elem, "subtype", "");
            const auto color = DataUtils::ParseXmlAttribute(elem, "color", "");
            etype.name = StringUtils::Join('.', slot, type, subtype, color);
        });
    });
}

void Map::LoadEquipmentDefinitionsFromFile(const std::filesystem::path& src) {
    namespace FS = std::filesystem;
    if(!FS::exists(src)) {
        std::ostringstream ss;
        ss << "Equipment Definitions file at " << src << " could not be found.";
        ERROR_AND_DIE(ss.str().c_str());
    }
    tinyxml2::XMLDocument doc;
    auto xml_result = doc.LoadFile(src.string().c_str());
    if(xml_result != tinyxml2::XML_SUCCESS) {
        std::ostringstream ss;
        ss << "Equipment Definitions at " << src << " failed to load.";
        ERROR_AND_DIE(ss.str().c_str());
    }
    if(auto* xml_root = doc.RootElement()) {
        DataUtils::ValidateXmlElement(*xml_root, "equipmentDefinitions", "spritesheet,equipmentDefinition", "");
        auto* xml_spritesheet = xml_root->FirstChildElement("spritesheet");
        _equipment_sheet = _renderer.CreateSpriteSheet(*xml_spritesheet);
        DataUtils::ForEachChildElement(*xml_root, "equipmentDefinition",
            [this](const XMLElement& elem) {
            EquipmentDefinition::CreateEquipmentDefinition(_renderer, elem, _equipment_sheet);
        });
    }
}

std::vector<EntityType*> Map::GetEntityTypesByName(const std::string& name) {
    auto range = _entity_types.equal_range(name);
    std::vector<EntityType*> results{};
    results.reserve(std::distance(range.first, range.second));
    for(auto iter = range.first; iter != range.second; ++iter) {
        results.push_back(iter->second.get());
    }
    return results;
}

std::vector<EquipmentType*> Map::GetEquipmentTypesByName(const std::string& name) {
    auto range = _equipment_types.equal_range(name);
    std::vector<EquipmentType*> results{};
    results.reserve(std::distance(range.first, range.second));
    for (auto iter = range.first; iter != range.second; ++iter) {
        results.push_back(iter->second.get());
    }
    return results;
}

void Map::PlaceEntitiesOnMap(const XMLElement& elem) {
    if(auto* xml_entity_map = elem.FirstChildElement("entityMap")) {
        const std::size_t entity_count = DataUtils::GetChildElementCount(*xml_entity_map);
        _entities.reserve(entity_count);
        DataUtils::ForEachChildElement(*xml_entity_map, "",
            [this](const XMLElement& elem) {
            auto name = DataUtils::GetElementName(elem);
            DataUtils::ValidateXmlElement(elem, name, "", "start");
            const auto start = DataUtils::ParseXmlAttribute(elem, "start", IntVector2{});
            const auto typeName = DataUtils::GetElementName(elem);
            auto types = GetEntityTypesByName(typeName);
            for(const auto type : types) {
                Entity* entity = nullptr;
                bool is_player = name == "player";
                if(is_player) {
                    entity = new Actor(type->definition);
                } else {
                    entity = new Entity(type->definition);
                }
                entity->name = typeName;
                entity->sprite = entity->def->GetSprite();
                entity->map = this;
                entity->layer = this->GetLayer(0);
                entity->SetPosition(start);
                _entities.push_back(entity);
                if(is_player) {
                    player = dynamic_cast<Actor*>(_entities.back());
                }
            }
        });
    }
}

