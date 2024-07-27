#pragma once

namespace big
{
	struct gravitytype
	{
		const char* name;
		float value;
	};

	constexpr const static auto gravity_presets = std::to_array<gravitytype>({
		{"GRAVITY_MOON", 1.6f},
		{"GRAVITY_SUN", 274.f},
		{"GRAVITY_PLUTO", 0.6f},
		{"GRAVITY_SPACE", 0.f},
		{"GRAVITY_MERCURY", 3.7f},
		{"GRAVITY_VENUS", 8.9f},
		{"GRAVITY_EARTH", 9.8f},
		{"GRAVITY_MARS", 3.7f},
		{"GRAVITY_JUPITER", 24.8f},
		{"GRAVITY_SATURN", 10.5f},
		{"GRAVITY_URANUS", 8.7f},
		{"GRAVITY_NEPTUNE", 11.2f}
	});
}
