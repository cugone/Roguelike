#include "Game/Equipment.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"

#include "Engine/Renderer/AnimatedSprite.hpp"

#include "Game/EquipmentDefinition.hpp"
#include "Game/Entity.hpp"

#include <sstream>

Equipment::Equipment(Renderer& renderer, const XMLElement& elem) noexcept
    : _renderer(renderer)
{
    LoadFromXml(elem);
}

void Equipment::BeginFrame() {

}

void Equipment::Update(TimeUtils::FPSeconds deltaSeconds) {
    sprite->Update(deltaSeconds);
}

void Equipment::Render(std::vector<Vertex3D>& verts, std::vector<unsigned int>& ibo, const Rgba& layer_color, size_t layer_index) const {

    if(!sprite || (owner && owner->IsInvisible())) {
        return;
    }

    const auto position = owner->GetPosition();
    const auto color = owner->color;

    const auto& coords = sprite->GetCurrentTexCoords();

    const auto vert_left = position.x + 0.0f;
    const auto vert_right = position.x + 1.0f;
    const auto vert_top = position.y + 0.0f;
    const auto vert_bottom = position.y + 1.0f;

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

void Equipment::EndFrame() {

}

void Equipment::ApplyStatModifer() {
    if (owner) {
        owner->AdjustStatModifiers(stats);
    }
}

void Equipment::RemoveStatModifer() {
    if (owner) {
        owner->AdjustStatModifiers(-stats);
    }
}

void Equipment::LoadFromXml(const XMLElement& elem) {
    DataUtils::ValidateXmlElement(elem, "equipment", "definition", "name");
    name = DataUtils::ParseXmlAttribute(elem, "name", name);
    auto xml_definition = elem.FirstChildElement("definition");
    auto definition_name = ParseEquipmentDefinitionName(*xml_definition);
    def = EquipmentDefinition::GetEquipmentDefinitionByName(definition_name);
    sprite = def->GetSprite();
}

std::string Equipment::ParseEquipmentDefinitionName(const XMLElement& xml_definition) {
    return StringUtils::Join(std::vector<std::string>{
        DataUtils::ParseXmlAttribute(xml_definition, "slot", std::string{})
            , DataUtils::ParseXmlAttribute(xml_definition, "type", std::string{})
            , DataUtils::ParseXmlAttribute(xml_definition, "subtype", std::string{})
            , DataUtils::ParseXmlAttribute(xml_definition, "color", std::string{})
    }, '.', false);
}
