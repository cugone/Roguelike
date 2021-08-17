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

constexpr int min_light_value = 0;
constexpr int day_light_value = 15;
constexpr int night_light_value = 3;
constexpr int max_light_value = 15;
constexpr float min_light_scale = 0.0f;
constexpr float max_light_scale = 1.0f;

constexpr uint32_t tile_coords_y_mask           = 0b0000'0000'1111'1111'0000'0000'0000'0000;
constexpr uint32_t tile_coords_x_mask           = 0b0000'0000'0000'0000'1111'1111'0000'0000;
constexpr uint32_t tile_coords_mask             = tile_coords_y_mask | tile_coords_x_mask;
constexpr uint32_t tile_flags_light_mask        = 0b0000'0000'0000'0000'0000'0000'0000'1111;
constexpr uint32_t tile_flags_can_see_mask      = 0b0000'0000'0000'0000'0000'0000'1000'0000;
constexpr uint32_t tile_flags_opaque_mask       = 0b0000'0000'0000'0000'0000'0000'0100'0000;
constexpr uint32_t tile_flags_solid_mask        = 0b0000'0000'0000'0000'0000'0000'0010'0000;
constexpr uint32_t tile_flags_dirty_light_mask  = 0b0000'0000'0000'0000'0000'0000'0001'0000;
constexpr uint32_t tile_flags_opaque_solid_mask = tile_flags_opaque_mask | tile_flags_solid_mask;
constexpr uint32_t tile_flags_mask              = tile_flags_opaque_solid_mask | tile_flags_dirty_light_mask;
constexpr uint32_t tile_y_bits = 8;
constexpr uint32_t tile_x_bits = 8;
constexpr uint32_t tile_flags_bits = 4;
constexpr uint32_t tile_light_bits = 4;
constexpr uint32_t tile_y_offset = 16;
constexpr uint32_t tile_x_offset = 8;
constexpr uint32_t tile_flags_offset = 4;
constexpr uint32_t tile_light_offset = 0;
