#pragma once

#include <elle/core/lathe_controller.hpp>

#include <expected>
#include <string>

namespace elle::linuxcnc {

/// The `lathe` HAL component: the pins lathe.hal wires to the step generators, encoders and the
/// manual/motion multiplexers. Same pin names, types and directions as the original Python
/// component, so the HAL wiring is unchanged.
class LatheComponent {
public:
    /// Registers the component and its pins. The component is not ready until `ready()`.
    [[nodiscard]] static auto create() -> std::expected<LatheComponent, std::string>;

    LatheComponent(const LatheComponent&) = delete;
    auto operator=(const LatheComponent&) -> LatheComponent& = delete;
    LatheComponent(LatheComponent&& other) noexcept;
    auto operator=(LatheComponent&& other) noexcept -> LatheComponent&;
    ~LatheComponent();

    /// Tells HAL that all pins exist, so `net` commands referencing them can run.
    [[nodiscard]] auto ready() const -> std::expected<void, std::string>;

    [[nodiscard]] auto read() const -> core::MachineSample;
    auto write(const core::PinOutputs& outputs) -> void;

private:
    struct Pins;
    LatheComponent(int component_id, Pins* pins) noexcept;

    int component_id_{-1};
    Pins* pins_{}; ///< in HAL shared memory, owned by HAL
};

} // namespace elle::linuxcnc
