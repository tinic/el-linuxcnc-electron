#include <elle/linuxcnc/lathe_component.hpp>

#include <elle/core/lathe_controller.hpp>

#include <expected>
#include <format>
#include <string>
#include <utility>

#include <hal.h>

namespace elle::linuxcnc {

namespace {

constexpr const char* component_name = "lathe";

auto pin_name(const char* pin) -> std::string
{
    return std::format("{}.{}", component_name, pin);
}

auto new_pin(const char* pin, hal_pin_dir_t direction, hal_float_t** slot, int component_id)
    -> bool
{
    return hal_pin_float_new(pin_name(pin).c_str(), direction, slot, component_id) == 0;
}

auto new_pin(const char* pin, hal_pin_dir_t direction, hal_bit_t** slot, int component_id) -> bool
{
    return hal_pin_bit_new(pin_name(pin).c_str(), direction, slot, component_id) == 0;
}

auto new_pin(const char* pin, hal_pin_dir_t direction, hal_u32_t** slot, int component_id) -> bool
{
    return hal_pin_u32_new(pin_name(pin).c_str(), direction, slot, component_id) == 0;
}

} // namespace

/// Pointers to the pins' storage, as HAL hands them out. Lives in HAL shared memory.
struct LatheComponent::Pins {
    hal_bit_t* control_source{};
    hal_float_t* position_z{};
    hal_float_t* position_x{};
    hal_float_t* position_a{};
    hal_float_t* speed_rps{};
    hal_float_t* forward_z{};
    hal_float_t* forward_x{};
    hal_bit_t* enable_z{};
    hal_bit_t* enable_x{};
    hal_bit_t* enable_stepper_z{};
    hal_bit_t* enable_stepper_x{};
    hal_float_t* position_z_encoder{};
    hal_float_t* position_x_encoder{};
    hal_float_t* offset_z_encoder{};
    hal_float_t* offset_z_stepper{};
    hal_float_t* offset_x_encoder{};
    hal_float_t* offset_x_stepper{};
    hal_bit_t* control_z_type{};
    hal_bit_t* control_x_type{};
    hal_float_t* velocity_z_cmd{};
    hal_float_t* velocity_x_cmd{};
    hal_u32_t* reset_z{};
    hal_u32_t* reset_x{};
    hal_float_t* scale_encoder_z{};
    hal_float_t* scale_encoder_x{};
};

auto LatheComponent::create() -> std::expected<LatheComponent, std::string>
{
    const int component_id = hal_init(component_name);
    if (component_id < 0) {
        return std::unexpected{std::format("hal_init(\"{}\") failed: {}", component_name,
                                           component_id)};
    }
    auto* pins = static_cast<Pins*>(hal_malloc(sizeof(Pins)));
    if (pins == nullptr) {
        hal_exit(component_id);
        return std::unexpected{std::string{"hal_malloc failed"}};
    }
    LatheComponent component{component_id, pins};

    const int id = component_id;
    const bool created =
        new_pin("control_source", HAL_OUT, &pins->control_source, id) &&
        new_pin("position_z", HAL_IN, &pins->position_z, id) &&
        new_pin("position_x", HAL_IN, &pins->position_x, id) &&
        new_pin("position_a", HAL_IN, &pins->position_a, id) &&
        new_pin("speed_rps", HAL_IN, &pins->speed_rps, id) &&
        new_pin("forward_z", HAL_OUT, &pins->forward_z, id) &&
        new_pin("forward_x", HAL_OUT, &pins->forward_x, id) &&
        new_pin("enable_z", HAL_OUT, &pins->enable_z, id) &&
        new_pin("enable_x", HAL_OUT, &pins->enable_x, id) &&
        new_pin("enable_stepper_z", HAL_OUT, &pins->enable_stepper_z, id) &&
        new_pin("enable_stepper_x", HAL_OUT, &pins->enable_stepper_x, id) &&
        new_pin("position_z_encoder", HAL_IN, &pins->position_z_encoder, id) &&
        new_pin("position_x_encoder", HAL_IN, &pins->position_x_encoder, id) &&
        new_pin("offset_z_encoder", HAL_OUT, &pins->offset_z_encoder, id) &&
        new_pin("offset_z_stepper", HAL_OUT, &pins->offset_z_stepper, id) &&
        new_pin("offset_x_encoder", HAL_OUT, &pins->offset_x_encoder, id) &&
        new_pin("offset_x_stepper", HAL_OUT, &pins->offset_x_stepper, id) &&
        new_pin("control_z_type", HAL_OUT, &pins->control_z_type, id) &&
        new_pin("control_x_type", HAL_OUT, &pins->control_x_type, id) &&
        new_pin("velocity_z_cmd", HAL_OUT, &pins->velocity_z_cmd, id) &&
        new_pin("velocity_x_cmd", HAL_OUT, &pins->velocity_x_cmd, id) &&
        new_pin("reset_z", HAL_OUT, &pins->reset_z, id) &&
        new_pin("reset_x", HAL_OUT, &pins->reset_x, id) &&
        new_pin("scale_encoder_z", HAL_OUT, &pins->scale_encoder_z, id) &&
        new_pin("scale_encoder_x", HAL_OUT, &pins->scale_encoder_x, id);
    if (!created) {
        return std::unexpected{std::string{"creating the lathe HAL pins failed"}};
    }
    return component;
}

LatheComponent::LatheComponent(int component_id, Pins* pins) noexcept
    : component_id_{component_id}
    , pins_{pins}
{
}

LatheComponent::LatheComponent(LatheComponent&& other) noexcept
    : component_id_{std::exchange(other.component_id_, -1)}
    , pins_{std::exchange(other.pins_, nullptr)}
{
}

auto LatheComponent::operator=(LatheComponent&& other) noexcept -> LatheComponent&
{
    if (this != &other) {
        if (component_id_ >= 0) {
            hal_exit(component_id_);
        }
        component_id_ = std::exchange(other.component_id_, -1);
        pins_ = std::exchange(other.pins_, nullptr);
    }
    return *this;
}

LatheComponent::~LatheComponent()
{
    if (component_id_ >= 0) {
        hal_exit(component_id_);
    }
}

auto LatheComponent::ready() -> std::expected<void, std::string>
{
    const int result = hal_ready(component_id_);
    if (result != 0) {
        return std::unexpected{std::format("hal_ready failed: {}", result)};
    }
    return {};
}

auto LatheComponent::read() const -> core::MachineSample
{
    return {.position_z = *pins_->position_z,
            .position_x = *pins_->position_x,
            .spindle_revs = *pins_->position_a,
            .spindle_rps = *pins_->speed_rps,
            .stepper_z = *pins_->position_z_encoder,
            .stepper_x = *pins_->position_x_encoder};
}

auto LatheComponent::write(const core::PinOutputs& outputs) -> void
{
    *pins_->control_source = outputs.control_source;
    *pins_->forward_z = outputs.forward_z;
    *pins_->forward_x = outputs.forward_x;
    *pins_->enable_z = outputs.enable_z;
    *pins_->enable_x = outputs.enable_x;
    *pins_->enable_stepper_z = outputs.enable_stepper_z;
    *pins_->enable_stepper_x = outputs.enable_stepper_x;
    *pins_->offset_z_encoder = outputs.offset_z_encoder;
    *pins_->offset_z_stepper = outputs.offset_z_stepper;
    *pins_->offset_x_encoder = outputs.offset_x_encoder;
    *pins_->offset_x_stepper = outputs.offset_x_stepper;
    *pins_->control_z_type = outputs.control_z_type;
    *pins_->control_x_type = outputs.control_x_type;
    *pins_->velocity_z_cmd = outputs.velocity_z_cmd;
    *pins_->velocity_x_cmd = outputs.velocity_x_cmd;
    *pins_->reset_z = outputs.reset_z;
    *pins_->reset_x = outputs.reset_x;
    *pins_->scale_encoder_z = outputs.scale_encoder_z;
    *pins_->scale_encoder_x = outputs.scale_encoder_x;
}

} // namespace elle::linuxcnc
