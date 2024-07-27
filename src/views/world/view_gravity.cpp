#include "views/view.hpp"
#include "core/data/gravity_presets.hpp"

namespace big
{
	void view::gravity()
	{
		components::command_checkbox<"modifygravity">();

		if (g.world.gravity.modify_gravity)
		{
			ImGui::SliderFloat("GRAVITY_LEVEL"_T.data(), &g.world.gravity.current_gravity, 0.f, 1000.f, "%.1f");

			static int selected_lunar_preset = 0;
			if (ImGui::BeginCombo("GRAVITY_LUNAR_PRESETS"_T.data(),
			        g_translation_service.get_translation(gravity_presets[selected_lunar_preset].name).data()))
			{
				for (int i = 0; i < gravity_presets.size(); i++)
				{
					if (ImGui::Selectable(
					        g_translation_service.get_translation(gravity_presets[i].name).data(),
					        g.world.local_weather == i))
					{
						g.world.gravity.current_gravity = gravity_presets[i].value;
						selected_lunar_preset = i;
					}
				}

				ImGui::EndCombo();
			}
		}
	}
}