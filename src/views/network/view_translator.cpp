﻿#include "views/view.hpp"
#include "util/chat.hpp"
#include "backend/bool_command.hpp"
#include "services/api/api_service.hpp"

#include <imgui_internal.h>


namespace big
{
	struct DeeplTargetLanguage
	{
		const char* TargetLanguageType;
		const char* TargetLanguageName;
	};
	struct BingTargetLanguage
	{
		const char* TargetLanguageType;
		const char* TargetLanguageName;
	};
	struct ServiceProvider
	{
		int ProviderID;
		const char* ProviderName;
	};

	bool_command chat_translate("translatechat", "translate chat message", "translate chat message", g.session.translatechat);
	bool_command chat_translate_hide_d("hideduplicate", "bypass same language", "Do not translate when source and target languages ​​are the same",
	    g.session.hideduplicate);

	void view::translator()
	{
		components::sub_title("Chat Translation");

		static const auto Provider = std::to_array<ServiceProvider>({{0, "Microsoft"}, {1, "DeepLx"}});

		if (ImGui::BeginCombo("Service Provider##ServiceProvider", Provider[g.session.t_service_provider].ProviderName))
		{
			for (const auto& [id, name] : Provider)
			{
				components::selectable(name, false, [&id] {
					g.session.t_service_provider = id;
				});
			}
			ImGui::EndCombo();
		}

		components::button("testmsg", [] {
			ChatMessage messagetoadd{"testsender", "This is a test message"};
			MsgQueue.push(messagetoadd);
		});
		components::command_checkbox<"translatechat">();
		components::command_checkbox<"hideduplicate">();

		if (ImGui::CollapsingHeader("Bing Settings"_T.data()))
		{
			static const auto BingLTargetLang = std::to_array<BingTargetLanguage>({{"ar", "Arabic"}, {"az", "Azerbaijani"}, {"bn", "Bangla"}, {"bg", "Bulgarian"}, {"zh-Hans", "Chinese Simplified"}, {"zh-Hant", "Chinese Traditional"}, {"hr", "Croatian"}, {"cs", "Czech"}, {"da", "Danish"}, {"de", "German"}, {"el", "Greek"}, {"en", "English"}, {"es", "Spanish"}, {"et", "Estonian"}, {"fi", "Finnish"}, {"fr", "French"}, {"hu", "Hungarian"}, {"id", "Indonesian"}, {"it", "Italian"}, {"ja", "Japanese"}, {"ko", "Korean"}, {"lt", "Lithuanian"}, {"lv", "Latvian"}, {"nb", "Norwegian(Bokmål)"}, {"nl", "Dutch"}, {"pl", "Polish"}, {"pt", "Portuguese"}, {"pt-br", "Portuguese(Brazilian)"}, {"ro", "Romanian"}, {"ru", "Russian"}, {"sk", "Slovak"}, {"sl", "Slovenian"}, {"sv", "Swedish"}, {"th", "Thai"}, {"tr", "Turkish"}, {"uk", "Ukrainian"}, {"vi", "Vietnamese"}});

			if (ImGui::BeginCombo("TargetLanguage##BingTargetLanguage", g.session.Bing_target_lang.c_str()))
			{
				for (const auto& [type, name] : BingLTargetLang)
				{
					components::selectable(name, false, [&type] {
						g.session.Bing_target_lang = type;
					});
				}
				ImGui::EndCombo();
			}
			components::button("Refresh token", [] {
				ms_token_str = "";
			});

		}

		if (ImGui::CollapsingHeader("DeepL Settings"_T.data()))
		{
			static const auto DeepLTargetLang = std::to_array<DeeplTargetLanguage>({{"AR", "Arabic"}, {"BG", "Bulgarian"}, {"CS", "Czech"}, {"DA", "Danish"}, {"DE", "German"}, {"EL", "Greek"}, {"EN", "English"}, {"EN-GB", "English(British)"}, {"EN-US", "English(American)"}, {"ES", "Spanish"}, {"ET", "Estonian"}, {"FI", "Finnish"}, {"FR", "French"}, {"HU", "Hungarian"}, {"ID", "Indonesian"}, {"IT", "Italian"}, {"JA", "Japanese"}, {"KO", "Korean"}, {"LT", "Lithuanian"}, {"LV", "Latvian"}, {"NB", "Norwegian(Bokmål)"}, {"NL", "Dutch"}, {"PL", "Polish"}, {"PT", "Portuguese"}, {"PT-BR", "Portuguese(Brazilian)"}, {"PT-PT", "Portuguese(Others)"}, {"RO", "Romanian"}, {"RU", "Russian"}, {"SK", "Slovak"}, {"SL", "Slovenian"}, {"SV", "Swedish"}, {"TR", "Turkish"}, {"UK", "Ukrainian"}, {"ZH", "Chinese(simplified)"}});

			if (ImGui::BeginCombo("TargetLanguage##DeepLTargetLangSwitcher", g.session.DeepL_target_lang.c_str()))
			{
				for (const auto& [type, name] : DeepLTargetLang)
				{
					components::selectable(name, false, [&type] {
						g.session.DeepL_target_lang = type;
					});
				}
				ImGui::EndCombo();
			}

		}
	}
}