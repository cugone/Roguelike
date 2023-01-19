#pragma once

#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/FileLogger.hpp"
#include "Engine/Core/Config.hpp"
#include "Engine/Core/Console.hpp"
#include "Engine/Core/JobSystem.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/UI/UISystem.hpp"

#include <filesystem>

constexpr uint8_t min_map_width{1u};
constexpr uint8_t min_map_height{1u};
constexpr uint8_t max_map_width{255u};
constexpr uint8_t max_map_height{255u};
constexpr int min_light_value{0};
constexpr int day_light_value{15};
constexpr int night_light_value{3};
constexpr int max_light_value{15};
constexpr float min_light_scale{0.0f};
constexpr float max_light_scale{1.0f};

constexpr uint32_t tile_coords_y_mask{0b0000'0000'1111'1111'0000'0000'0000'0000u};
constexpr uint32_t tile_coords_x_mask{0b0000'0000'0000'0000'1111'1111'0000'0000u};
constexpr uint32_t tile_coords_mask{tile_coords_y_mask | tile_coords_x_mask};
constexpr uint32_t tile_flags_light_mask{0b0000'0000'0000'0000'0000'0000'0000'1111u};
constexpr uint32_t tile_flags_can_see_mask{0b0000'0000'0000'0000'0000'0000'1000'0000u};
constexpr uint32_t tile_flags_opaque_mask{0b0000'0000'0000'0000'0000'0000'0100'0000u};
constexpr uint32_t tile_flags_solid_mask{0b0000'0000'0000'0000'0000'0000'0010'0000u};
constexpr uint32_t tile_flags_dirty_light_mask{0b0000'0000'0000'0000'0000'0000'0001'0000u};
constexpr uint32_t tile_flags_opaque_solid_mask{tile_flags_opaque_mask | tile_flags_solid_mask};
constexpr uint32_t tile_flags_mask{tile_flags_opaque_solid_mask | tile_flags_dirty_light_mask};
constexpr uint32_t tile_y_bits{8u};
constexpr uint32_t tile_x_bits{8u};
constexpr uint32_t tile_flags_bits{4u};
constexpr uint32_t tile_light_bits{4u};
constexpr uint32_t tile_y_offset{16u};
constexpr uint32_t tile_x_offset{8u};
constexpr uint32_t tile_flags_offset{4u};
constexpr uint32_t tile_light_offset{0u};

static inline const std::filesystem::path default_tile_definition_src{ "Data/Definitions/Tiles.xml" };
static inline const std::filesystem::path default_item_definition_src{ "Data/Definitions/Items.xml" };
static inline const std::filesystem::path default_entities_definition_src{ "Data/Definitions/Entities.xml" };
static inline const std::filesystem::path default_adventure_src{ "Data/Maps/Adventure.xml" };
static inline const std::filesystem::path default_ui_src{ "Data/Definitions/UI.xml" };
