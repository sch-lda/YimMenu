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
	struct GoogleTargetLanguage
	{
		const char* TargetLanguageType;
		const char* TargetLanguageName;
	};
	struct ServiceProvider
	{
		int ProviderID;
		const char* ProviderName;
	};

	bool_command chat_translate("translatechat", "聊天翻译", "翻译传入的聊天消息", g.session.translatechat);
	bool_command chat_translate_hide_d("bypasssamelang", "绕过源语言", "当源语言与目标语言相同时不输出翻译",
	    g.session.chat_translator_bypass);

	void view::translator()
	{
		components::sub_title("Chat Translation");
		components::small_text("前三个提供商基于逆向分析,并非官方免费开放的接口,使用前请注意您所在国家和可能导致的法律问题!");

		static const auto Provider = std::to_array<ServiceProvider>({{0, "Microsoft(无门槛)"}, {1, "Google(需挂代理)"}, {2, "DeepLx(需要DeepLx程序)"}, {3, "OpenAI(需要token)"}});

		if (ImGui::BeginCombo("翻译服务提供商##ServiceProvider", Provider[g.session.t_service_provider].ProviderName))
		{
			for (const auto& [id, name] : Provider)
			{
				components::selectable(name, false, [&id] {
					g.session.t_service_provider = id;
				});
			}
			ImGui::EndCombo();
		}

		components::button("发送测试消息", [] {
			ChatMessage messagetoadd{"testsender", "This is a test message"};
			MsgQueue.push(messagetoadd);
		});
		components::command_checkbox<"translatechat">();
		components::command_checkbox<"bypasssamelang">();

		components::sub_title("翻译结果输出选项");
		ImGui::Checkbox("显示但不发送(进自己可见)", &g.session.chat_translator_draw);
		ImGui::Checkbox("发送到公屏聊天", &g.session.chat_translator_send);
		ImGui::SameLine();
		ImGui::Checkbox("仅团队", &g.session.chat_translator_send_team);
		ImGui::Checkbox("输出到控制台", &g.session.chat_translator_print);

		if (ImGui::CollapsingHeader("微软必应设置"_T.data()))
		{
			static const auto BingTargetLang = std::to_array<BingTargetLanguage>({{"ar", "Arabic"}, {"az", "Azerbaijani"}, {"bn", "Bangla"}, {"bg", "Bulgarian"}, {"zh-Hans", "Chinese Simplified"}, {"zh-Hant", "Chinese Traditional"}, {"hr", "Croatian"}, {"cs", "Czech"}, {"da", "Danish"}, {"de", "German"}, {"el", "Greek"}, {"en", "English"}, {"es", "Spanish"}, {"et", "Estonian"}, {"fi", "Finnish"}, {"fr", "French"}, {"hu", "Hungarian"}, {"id", "Indonesian"}, {"it", "Italian"}, {"ja", "Japanese"}, {"ko", "Korean"}, {"lt", "Lithuanian"}, {"lv", "Latvian"}, {"nb", "Norwegian(Bokmål)"}, {"nl", "Dutch"}, {"pl", "Polish"}, {"pt", "Portuguese"}, {"pt-br", "Portuguese(Brazilian)"}, {"ro", "Romanian"}, {"ru", "Russian"}, {"sk", "Slovak"}, {"sl", "Slovenian"}, {"sv", "Swedish"}, {"th", "Thai"}, {"tr", "Turkish"}, {"uk", "Ukrainian"}, {"vi", "Vietnamese"}});

			if (ImGui::BeginCombo("目标语言##BingTargetLanguage", g.session.Bing_target_lang.c_str()))
			{
				for (const auto& [type, name] : BingTargetLang)
				{
					components::selectable(name, false, [&type] {
						g.session.Bing_target_lang = type;
					});
				}
				ImGui::EndCombo();
			}
			components::button("重新获取token", [] {
				ms_token_str = "";
			});

		}
		if (ImGui::CollapsingHeader("谷歌翻译设置"))
		{
			static const auto GoogleTargetLang = std::to_array<GoogleTargetLanguage>({{"ar", "Arabic"}, {"az", "Azerbaijani"}, {"bg", "Bulgarian"}, {"zh-CN", "Chinese Simplified"}, {"zh-TW", "Chinese Traditional"}, {"hr", "Croatian"}, {"cs", "Czech"}, {"da", "Danish"}, {"de", "German"}, {"el", "Greek"}, {"en", "English"}, {"es", "Spanish"}, {"et", "Estonian"}, {"fi", "Finnish"}, {"fr", "French"}, {"hu", "Hungarian"}, {"id", "Indonesian"}, {"it", "Italian"}, {"ja", "Japanese"}, {"ko", "Korean"}, {"lt", "Lithuanian"}, {"lv", "Latvian"}, {"n0", "Norwegian"}, {"nl", "Dutch"}, {"pl", "Polish"}, {"pt", "Portuguese"}, {"ro", "Romanian"}, {"ru", "Russian"}, {"sk", "Slovak"}, {"sl", "Slovenian"}, {"sv", "Swedish"}, {"th", "Thai"}, {"tr", "Turkish"}, {"uk", "Ukrainian"}, {"vi", "Vietnamese"}});

			if (ImGui::BeginCombo("目标语言##GoogleTargetLangSwitcher", g.session.Google_target_lang.c_str()))
			{
				for (const auto& [type, name] : GoogleTargetLang)
				{
					components::selectable(name, false, [&type] {
						g.session.Google_target_lang = type;
					});
				}
				ImGui::EndCombo();
			}
		}

		if (ImGui::CollapsingHeader("DeepLx设置"))
		{
			static const auto DeepLTargetLang = std::to_array<DeeplTargetLanguage>({{"AR", "Arabic"}, {"BG", "Bulgarian"}, {"CS", "Czech"}, {"DA", "Danish"}, {"DE", "German"}, {"EL", "Greek"}, {"EN", "English"}, {"EN-GB", "English(British)"}, {"EN-US", "English(American)"}, {"ES", "Spanish"}, {"ET", "Estonian"}, {"FI", "Finnish"}, {"FR", "French"}, {"HU", "Hungarian"}, {"ID", "Indonesian"}, {"IT", "Italian"}, {"JA", "Japanese"}, {"KO", "Korean"}, {"LT", "Lithuanian"}, {"LV", "Latvian"}, {"NB", "Norwegian(Bokmål)"}, {"NL", "Dutch"}, {"PL", "Polish"}, {"PT", "Portuguese"}, {"PT-BR", "Portuguese(Brazilian)"}, {"PT-PT", "Portuguese(Others)"}, {"RO", "Romanian"}, {"RU", "Russian"}, {"SK", "Slovak"}, {"SL", "Slovenian"}, {"SV", "Swedish"}, {"TR", "Turkish"}, {"UK", "Ukrainian"}, {"ZH", "Chinese(simplified)"}});
			
			components::input_text_with_hint("DeepLx URL", "http://127.0.0.1:1188/translate", g.session.DeepLx_url);
			
			if (ImGui::BeginCombo("目标语言##DeepLTargetLangSwitcher", g.session.DeepL_target_lang.c_str()))
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

		if (ImGui::CollapsingHeader("OpenAI Settings"_T.data()))
		{

			components::input_text_with_hint("OpenAI端点", "https://api.openai.com/", g.session.OpenAI_endpoint);
			components::input_text_with_hint("OpenAI token", "sk-*", g.session.OpenAI_key);
			components::input_text_with_hint("模型", "gpt-3.5-turbo", g.session.OpenAI_model);
			components::input_text_with_hint("目标语言##OpenAI", "Chinese", g.session.OpenAI_target_lang);

		}

	}
}