#include "font_mgr.hpp"

#include "fonts/fonts.hpp"
#include "renderer.hpp"
#include "thread_pool.hpp"

namespace big
{
	font_mgr::font_mgr(std::vector<std::pair<float, ImFont**>> extra_font_sizes) :
	    m_extra_font_sizes(extra_font_sizes),
	    m_require_extra(eAlphabetType::LATIN),
	    m_fonts({
	        {eAlphabetType::CHINESE, {"msyh.ttc", "msyh.ttf", "arial.ttf"}},
	        {eAlphabetType::CYRILLIC, {"msyh.ttc", "msyh.ttf", "arial.ttf"}},
	        {eAlphabetType::JAPANESE, {"msyh.ttc", "msyh.ttf", "arial.ttf"}},
	        {eAlphabetType::KOREAN, {"malgun.ttf", "arial.ttf"}},
	        {eAlphabetType::TURKISH, {"msyh.ttc", "msyh.ttf", "arial.ttf"}},
	    })
	{
	}

	bool font_mgr::can_use()
	{
		return m_update_lock.try_lock();
	}

	void font_mgr::release_use()
	{
		m_update_lock.unlock();
	}

	void font_mgr::rebuild()
	{
		m_update_lock.lock();

		g_renderer.pre_reset();

		const auto extra_font_file = get_available_font_file_for_alphabet_type();
		if (m_require_extra != eAlphabetType::LATIN && !extra_font_file.exists())
		{
			LOG(WARNING) << "Could not find an appropriate font for the current language!";
		}
		const auto extra_glyph_range = get_imgui_alphabet_type();

		auto& io = ImGui::GetIO();
		io.Fonts->Clear();

		// default font size
		{
			ImFontConfig fnt_cfg{};
			fnt_cfg.FontDataOwnedByAtlas = false;
			strcpy(fnt_cfg.Name, "Fnt20px");

			io.Fonts->AddFontFromMemoryTTF(const_cast<uint8_t*>(font_storopia),
			    sizeof(font_storopia),
			    20.f,
			    &fnt_cfg,
			    io.Fonts->GetGlyphRangesDefault());
			if (m_require_extra != eAlphabetType::LATIN && extra_font_file.exists())
			{
				fnt_cfg.MergeMode = true;
				io.Fonts->AddFontFromFileTTF(extra_font_file.get_path().string().c_str(), 20.f, &fnt_cfg, extra_glyph_range);
			}
			io.Fonts->Build();
		}

		// any other font sizes we need to support
		for (auto [size, font_ptr] : m_extra_font_sizes)
		{
			ImFontConfig fnt_cfg{};
			fnt_cfg.FontDataOwnedByAtlas = false;
			strcpy(fnt_cfg.Name, std::format("Fnt{}px", (int)size).c_str());

			*font_ptr = io.Fonts->AddFontFromMemoryTTF(const_cast<uint8_t*>(font_storopia),
			    sizeof(font_storopia),
			    size,
			    &fnt_cfg,
			    io.Fonts->GetGlyphRangesDefault());
			if (m_require_extra != eAlphabetType::LATIN && extra_font_file.exists())
			{
				fnt_cfg.MergeMode = true;
				io.Fonts->AddFontFromFileTTF(extra_font_file.get_path().string().c_str(), size, &fnt_cfg, extra_glyph_range);
			}
			io.Fonts->Build();
		}

		// icons blueh
		{
			ImFontConfig font_icons_cfg{};
			font_icons_cfg.FontDataOwnedByAtlas = false;
			std::strcpy(font_icons_cfg.Name, "Icons");
			g.window.font_icon = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(const_cast<uint8_t*>(font_icons), sizeof(font_icons), 24.f, &font_icons_cfg);
		}

		g_renderer.post_reset();

		m_update_lock.unlock();
	}

	void font_mgr::update_required_alphabet_type(eAlphabetType type)
	{
		m_require_extra = type;

		g_thread_pool->push([this] {
			rebuild();
		});
	}

	file font_mgr::get_available_font_file_for_alphabet_type()
	{
		static const auto fonts_folder = std::filesystem::path(std::getenv("SYSTEMROOT")) / "Fonts";

		const auto& fonts = m_fonts.find(m_require_extra);
		if (fonts == m_fonts.end())
			return {};
		for (const auto& font : fonts->second)
		{
			auto font_file = file(fonts_folder / font);
			if (font_file.exists())
			{
				return font_file;
			}
		}
		return {};
	}

	const ImWchar* font_mgr::GetGlyphRangesChineseSimplifiedOfficial()
	{
		// Store all official characters for Simplified Chinese.
		// Sourced from https://en.wikipedia.org/wiki/Table_of_General_Standard_Chinese_Characters
		// (Stored as accumulative offsets from the initial unicode codepoint 0x4E00. This encoding is designed to helps us compact the source code size.)
		static const short accumulative_offsets_from_0x4E00[] = {0, 1, 2, 4, 1, 1, 1, 1, 2, 1, 1, 1, 1, 2, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 3, 2, 1, 1, 1, 3, 1, 2, 3, 2, 2, 4, 1, 1, 1, 2, 1, 4, 1, 2, 3, 1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 2, 2, 1, 3, 1, 1, 1, 1, 1, 5, 3, 7, 1, 2, 11, 4, 4, 2, 1, 2, 1, 2, 1, 2, 1, 1, 1, 1, 3, 1, 1, 1, 3, 1, 2, 1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 1, 4, 1, 2, 1, 4, 5, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 3, 2, 1, 1, 1, 1, 1, 1, 5, 1, 2, 2, 1, 1, 3, 2, 1, 1, 4, 2, 3, 1, 1, 4, 2, 2, 2, 8, 1, 3, 1, 1, 1, 1, 6, 1, 1, 1, 1, 3, 1, 1, 2, 2, 1, 1, 1, 3, 1, 4, 1, 4, 2, 2, 2, 2, 1, 6, 3, 1, 1, 5, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 1, 1, 4, 3, 3, 1, 3, 2, 1, 5, 1, 2, 1, 4, 1, 2, 2, 1, 2, 2, 2, 2, 3, 2, 1, 1, 3, 2, 3, 3, 2, 1, 1, 1, 1, 1, 2, 2, 1, 6, 1, 1, 8, 3, 1, 1, 1, 5, 4, 1, 1, 1, 6, 1, 2, 3, 1, 1, 1, 1, 2, 3, 2, 1, 1, 3, 1, 1, 2, 2, 2, 1, 2, 2, 4, 2, 6, 3, 2, 1, 1, 2, 2, 1, 2, 2, 2, 1, 1, 2, 3, 2, 4, 1, 2, 1, 1, 1, 1, 1, 13, 2, 2, 5, 4, 1, 1, 3, 2, 1, 6, 5, 2, 9, 7, 8, 1, 1, 1, 4, 2, 1, 1, 1, 5, 3, 5, 4, 7, 1, 9, 1, 2, 2, 1, 1, 3, 1, 5, 1, 2, 2, 4, 3, 7, 10, 6, 1, 4, 11, 2, 6, 1, 2, 1, 2, 2, 4, 2, 5, 2, 2, 2, 3, 2, 7, 3, 3, 7, 2, 9, 8, 5, 8, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 1, 1, 4, 1, 2, 1, 1, 4, 2, 6, 3, 2, 1, 1, 2, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 1, 2, 1, 1, 3, 5, 3, 1, 1, 2, 1, 5, 3, 2, 2, 2, 1, 4, 2, 2, 1, 7, 3, 1, 2, 1, 2, 1, 1, 4, 1, 1, 3, 4, 2, 1, 2, 2, 1, 1, 2, 2, 10, 2, 3, 1, 3, 7, 2, 2, 1, 1, 2, 3, 2, 1, 1, 1, 1, 1, 2, 1, 1, 2, 3, 1, 1, 2, 3, 1, 3, 1, 4, 1, 1, 1, 1, 1, 2, 3, 4, 1, 3, 1, 1, 1, 2, 1, 2, 3, 3, 1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 4, 2, 1, 1, 1, 1, 1, 2, 1, 3, 2, 5, 1, 1, 1, 3, 4, 2, 2, 1, 4, 1, 3, 3, 2, 6, 2, 2, 2, 4, 1, 1, 1, 3, 4, 2, 8, 2, 1, 1, 1, 1, 1, 1, 3, 2, 1, 1, 1, 1, 1, 4, 1, 1, 9, 2, 1, 2, 2, 4, 2, 2, 5, 2, 3, 1, 2, 1, 4, 1, 1, 3, 2, 12, 3, 2, 3, 2, 1, 3, 1, 1, 5, 1, 2, 5, 2, 1, 5, 1, 1, 2, 3, 1, 3, 1, 2, 3, 4, 4, 1, 2, 8, 1, 1, 3, 1, 1, 1, 2, 2, 2, 1, 1, 1, 4, 1, 2, 1, 1, 1, 1, 1, 1, 3, 2, 2, 1, 1, 1, 1, 1, 1, 2, 1, 4, 3, 1, 1, 1, 2, 1, 1, 2, 1, 1, 2, 4, 3, 2, 1, 1, 3, 2, 1, 1, 8, 3, 2, 3, 2, 3, 3, 1, 2, 1, 4, 1, 4, 9, 3, 1, 2, 1, 1, 3, 2, 1, 1, 1, 1, 1, 1, 3, 3, 2, 1, 1, 1, 2, 4, 1, 2, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 2, 1, 1, 4, 2, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 4, 2, 1, 1, 1, 1, 2, 3, 1, 1, 1, 2, 1, 1, 1, 1, 2, 1, 2, 1, 1, 2, 1, 2, 1, 1, 1, 2, 3, 3, 2, 2, 1, 3, 2, 2, 1, 1, 1, 1, 1, 1, 3, 1, 6, 1, 1, 2, 2, 9, 1, 1, 2, 1, 1, 1, 3, 1, 1, 3, 2, 2, 2, 5, 1, 2, 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 2, 6, 1, 2, 1, 1, 1, 1, 1, 1, 3, 2, 2, 1, 4, 3, 2, 2, 1, 1, 1, 2, 2, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 3, 1, 1, 2, 4, 1, 1, 1, 1, 1, 3, 1, 4, 8, 2, 1, 4, 5, 1, 2, 6, 1, 1, 3, 7, 5, 2, 1, 1, 3, 5, 2, 1, 1, 1, 2, 2, 2, 1, 4, 2, 1, 2, 2, 1, 2, 3, 1, 5, 1, 5, 1, 6, 2, 1, 2, 3, 1, 1, 1, 3, 2, 1, 1, 2, 5, 1, 1, 1, 3, 1, 2, 2, 1, 1, 1, 1, 3, 1, 1, 1, 1, 4, 2, 3, 4, 1, 1, 2, 1, 2, 8, 3, 2, 2, 3, 1, 1, 2, 2, 2, 2, 2, 1, 6, 1, 1, 1, 2, 3, 1, 1, 2, 1, 1, 1, 1, 2, 4, 2, 1, 1, 1, 2, 2, 1, 1, 1, 2, 2, 1, 1, 3, 3, 1, 2, 1, 1, 6, 1, 2, 1, 5, 2, 1, 3, 1, 1, 6, 2, 1, 1, 2, 1, 3, 1, 2, 2, 1, 3, 2, 3, 1, 1, 1, 1, 1, 1, 2, 2, 3, 2, 4, 2, 11, 1, 1, 5, 1, 3, 1, 1, 3, 4, 2, 2, 1, 3, 1, 1, 1, 1, 3, 2, 3, 2, 2, 1, 2, 1, 8, 1, 1, 1, 7, 1, 1, 3, 2, 14, 2, 3, 6, 1, 5, 3, 5, 6, 7, 1, 7, 3, 6, 1, 3, 1, 1, 1, 1, 2, 6, 1, 2, 3, 1, 3, 1, 4, 1, 3, 1, 1, 4, 1, 2, 2, 1, 1, 1, 2, 5, 1, 3, 2, 4, 3, 4, 5, 1, 1, 2, 1, 1, 1, 1, 3, 6, 1, 1, 3, 2, 2, 5, 3, 2, 1, 1, 1, 1, 1, 6, 3, 1, 2, 1, 1, 1, 1, 3, 2, 2, 1, 1, 1, 2, 2, 4, 4, 4, 1, 6, 1, 1, 2, 5, 1, 6, 1, 8, 5, 1, 1, 1, 1, 2, 1, 2, 2, 2, 1, 3, 7, 10, 1, 8, 3, 4, 2, 1, 3, 1, 1, 3, 2, 1, 4, 9, 2, 5, 1, 2, 1, 1, 1, 3, 5, 3, 1, 1, 2, 2, 2, 4, 4, 4, 5, 3, 3, 6, 6, 1, 3, 1, 15, 2, 4, 2, 1, 4, 3, 3, 1, 6, 4, 9, 1, 9, 9, 2, 1, 2, 2, 4, 1, 8, 1, 6, 3, 2, 1, 9, 1, 5, 5, 9, 5, 4, 2, 10, 2, 2, 3, 6, 1, 5, 5, 2, 1, 2, 1, 1, 3, 3, 3, 1, 3, 7, 3, 6, 2, 5, 1, 1, 3, 1, 2, 3, 1, 2, 2, 1, 2, 2, 1, 1, 1, 1, 1, 1, 2, 3, 3, 1, 1, 1, 2, 2, 3, 1, 2, 3, 1, 1, 2, 3, 1, 1, 1, 3, 1, 1, 1, 1, 2, 6, 2, 3, 2, 3, 3, 1, 5, 1, 2, 2, 1, 4, 4, 1, 1, 1, 2, 1, 1, 2, 3, 5, 1, 3, 1, 2, 4, 1, 5, 1, 1, 3, 1, 1, 1, 3, 1, 3, 1, 6, 2, 3, 8, 4, 1, 5, 1, 1, 1, 1, 3, 1, 2, 2, 1, 6, 2, 1, 2, 2, 2, 2, 11, 2, 4, 2, 2, 1, 1, 1, 1, 1, 1, 3, 5, 2, 5, 3, 1, 3, 1, 3, 2, 4, 8, 1, 2, 2, 6, 4, 1, 5, 3, 1, 11, 5, 8, 4, 1, 3, 10, 1, 1, 1, 3, 5, 19, 8, 1, 15, 3, 5, 1, 2, 3, 5, 1, 3, 1, 7, 3, 6, 2, 2, 2, 2, 6, 1, 2, 3, 3, 2, 6, 4, 20, 3, 10, 1, 13, 12, 4, 3, 9, 3, 13, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 3, 1, 1, 1, 1, 3, 1, 1, 1, 4, 1, 2, 2, 3, 2, 3, 4, 2, 2, 2, 1, 1, 2, 1, 3, 4, 2, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 4, 1, 3, 2, 3, 1, 1, 1, 2, 1, 4, 1, 1, 3, 2, 1, 1, 1, 5, 4, 2, 1, 10, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 2, 1, 2, 2, 5, 1, 2, 1, 1, 1, 1, 3, 2, 3, 2, 1, 1, 1, 1, 1, 2, 1, 1, 2, 3, 1, 1, 2, 2, 2, 1, 7, 1, 2, 5, 5, 3, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 4, 3, 2, 2, 1, 1, 1, 1, 1, 1, 6, 3, 2, 8, 1, 5, 2, 1, 6, 4, 4, 1, 3, 2, 1, 1, 2, 1, 6, 1, 1, 6, 2, 1, 1, 4, 2, 4, 1, 7, 2, 3, 7, 7, 7, 8, 2, 1, 1, 1, 2, 2, 3, 3, 6, 5, 2, 4, 1, 1, 3, 1, 7, 6, 2, 1, 3, 1, 9, 2, 3, 4, 1, 6, 9, 10, 3, 1, 1, 2, 8, 2, 3, 2, 12, 2, 1, 3, 5, 14, 2, 3, 18, 4, 26, 3, 2, 9, 4, 4, 7, 9, 1, 3, 1, 3, 1, 1, 1, 1, 2, 3, 1, 2, 1, 1, 1, 3, 6, 1, 3, 1, 1, 2, 1, 2, 4, 3, 1, 1, 3, 1, 1, 2, 1, 1, 1, 1, 1, 8, 1, 1, 4, 2, 1, 5, 3, 1, 1, 3, 1, 1, 3, 2, 2, 1, 7, 8, 1, 6, 3, 1, 2, 1, 1, 8, 7, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 5, 2, 1, 3, 1, 4, 1, 1, 2, 1, 1, 1, 1, 2, 1, 2, 2, 1, 1, 5, 1, 1, 4, 2, 8, 1, 1, 1, 1, 5, 3, 1, 6, 1, 1, 9, 3, 4, 1, 2, 2, 1, 2, 1, 5, 2, 2, 7, 3, 1, 3, 5, 1, 1, 1, 1, 1, 4, 2, 1, 4, 2, 2, 2, 2, 1, 3, 4, 1, 5, 1, 1, 2, 4, 2, 2, 4, 2, 2, 1, 2, 4, 6, 2, 4, 2, 4, 1, 2, 2, 1, 1, 4, 5, 2, 2, 1, 2, 1, 2, 1, 3, 1, 6, 2, 2, 1, 3, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 4, 1, 1, 3, 2, 1, 1, 3, 2, 3, 7, 1, 1, 3, 1, 7, 2, 2, 3, 1, 6, 2, 1, 6, 1, 2, 1, 1, 1, 4, 1, 1, 1, 4, 3, 1, 3, 3, 3, 1, 2, 4, 4, 3, 3, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 7, 1, 1, 3, 2, 1, 1, 3, 2, 1, 1, 3, 1, 4, 1, 1, 1, 1, 1, 1, 4, 6, 6, 1, 3, 2, 1, 1, 3, 5, 2, 3, 2, 1, 2, 4, 1, 8, 1, 1, 1, 3, 1, 1, 1, 1, 1, 2, 1, 3, 3, 2, 1, 1, 1, 1, 1, 2, 5, 1, 4, 3, 1, 5, 2, 2, 4, 1, 2, 2, 1, 3, 3, 2, 3, 1, 1, 2, 2, 1, 2, 2, 2, 3, 1, 1, 8, 1, 1, 3, 1, 6, 4, 3, 2, 2, 1, 2, 1, 1, 5, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 1, 2, 3, 1, 6, 1, 5, 2, 1, 5, 1, 5, 1, 5, 1, 1, 3, 1, 3, 1, 2, 1, 4, 20, 5, 4, 2, 1, 1, 2, 3, 4, 3, 2, 3, 5, 1, 4, 1, 6, 2, 5, 1, 1, 7, 4, 8, 1, 3, 2, 1, 3, 6, 10, 3, 1, 1, 2, 1, 6, 4, 8, 4, 5, 1, 1, 1, 1, 6, 1, 20, 12, 3, 1, 1, 1, 2, 2, 2, 1, 1, 6, 2, 2, 2, 1, 1, 1, 1, 1, 1, 3, 1, 1, 1, 2, 1, 4, 2, 1, 3, 5, 2, 2, 2, 2, 1, 1, 2, 1, 6, 1, 1, 1, 1, 2, 4, 1, 1, 2, 2, 1, 3, 1, 1, 1, 4, 3, 8, 3, 1, 2, 1, 1, 1, 1, 1, 1, 1, 3, 3, 3, 3, 2, 1, 1, 3, 1, 5, 1, 7, 1, 1, 1, 1, 1, 1, 1, 2, 1, 4, 1, 1, 1, 2, 1, 3, 3, 1, 5, 4, 4, 2, 1, 1, 2, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 3, 3, 1, 1, 1, 1, 1, 1, 1, 3, 1, 1, 1, 2, 2, 1, 3, 5, 1, 1, 1, 2, 1, 5, 1, 1, 5, 3, 5, 4, 1, 2, 1, 1, 1, 1, 2, 1, 1, 1, 2, 2, 1, 4, 3, 7, 1, 3, 1, 4, 1, 2, 1, 3, 2, 1, 1, 1, 1, 1, 5, 9, 1, 2, 1, 1, 4, 1, 1, 2, 2, 1, 1, 3, 4, 1, 3, 1, 2, 3, 2, 1, 4, 1, 1, 1, 2, 3, 1, 2, 4, 2, 1, 2, 5, 1, 1, 1, 2, 2, 1, 1, 1, 2, 2, 2, 3, 1, 3, 1, 2, 2, 2, 6, 2, 3, 4, 2, 1, 2, 4, 4, 1, 5, 1, 2, 6, 1, 3, 1, 5, 1, 2, 2, 1, 4, 2, 1, 1, 1, 3, 1, 5, 2, 2, 1, 3, 1, 2, 1, 4, 1, 2, 2, 1, 4, 1, 1, 3, 2, 1, 7, 2, 4, 3, 1, 3, 3, 1, 1, 1, 1, 2, 8, 2, 4, 6, 1, 8, 2, 4, 2, 4, 5, 1, 1, 1, 2, 5, 1, 1, 1, 2, 1, 8, 1, 1, 1, 2, 4, 5, 6, 1, 4, 2, 1, 1, 1, 2, 2, 3, 2, 1, 2, 2, 3, 1, 1, 1, 2, 1, 2, 3, 1, 2, 1, 4, 2, 4, 2, 4, 2, 2, 2, 2, 6, 4, 1, 1, 2, 3, 3, 1, 3, 2, 6, 3, 6, 3, 2, 4, 4, 1, 6, 1, 1, 5, 1, 1, 2, 1, 7, 2, 1, 2, 3, 1, 6, 3, 4, 3, 2, 4, 1, 1, 1, 1, 2, 2, 1, 4, 1, 3, 5, 1, 4, 2, 2, 1, 2, 1, 10, 1, 4, 4, 1, 4, 1, 2, 4, 2, 2, 1, 3, 2, 3, 1, 2, 2, 2, 1, 1, 2, 1, 7, 2, 3, 1, 4, 2, 1, 1, 1, 5, 1, 2, 1, 3, 4, 1, 9, 2, 3, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 5, 1, 1, 2, 1, 5, 2, 1, 3, 1, 2, 1, 2, 2, 1, 4, 1, 1, 4, 4, 2, 1, 5, 2, 1, 5, 2, 2, 3, 1, 1, 6, 2, 3, 1, 1, 6, 2, 1, 3, 3, 1, 2, 1, 1, 3, 3, 1, 1, 2, 3, 2, 2, 6, 1, 1, 4, 2, 1, 3, 4, 4, 2, 3, 1, 1, 3, 5, 5, 1, 6, 5, 5, 1, 3, 3, 3, 3, 4, 1, 5, 8, 3, 13, 3, 2, 1, 1, 3, 6, 3, 3, 4, 2, 1, 1, 3, 1, 1, 3, 2, 1, 1, 3, 5, 1, 2, 2, 3, 4, 1, 2, 4, 2, 2, 7, 1, 1, 2, 1, 1, 1, 2, 2, 3, 1, 5, 3, 3, 2, 1, 3, 2, 1, 3, 2, 1, 1, 1, 2, 3, 6, 2, 1, 1, 1, 4, 3, 2, 3, 2, 1, 1, 1, 1, 2, 2, 4, 1, 2, 1, 2, 3, 3, 2, 2, 5, 5, 2, 3, 2, 1, 1, 4, 1, 2, 2, 1, 2, 1, 2, 2, 1, 3, 2, 1, 1, 1, 9, 3, 4, 7, 1, 1, 1, 1, 1, 4, 1, 1, 2, 1, 1, 2, 2, 3, 4, 3, 3, 1, 1, 2, 1, 1, 8, 2, 1, 5, 2, 1, 1, 1, 1, 1, 2, 1, 2, 2, 3, 1, 6, 2, 2, 8, 1, 8, 1, 4, 1, 1, 3, 1, 1, 2, 1, 1, 1, 2, 1, 2, 1, 3, 2, 2, 1, 2, 1, 1, 12, 1, 1, 1, 1, 1, 1, 1, 1, 2, 9, 2, 1, 8, 2, 2, 2, 1, 9, 4, 4, 6, 2, 3, 4, 1, 1, 5, 2, 1, 3, 2, 10, 1, 1, 1, 2, 5, 2, 2, 5, 3, 2, 1, 2, 5, 1, 3, 4, 7, 2, 1, 1, 3, 4, 1, 1, 1, 5, 4, 6, 2, 1, 2, 2, 13, 1, 4, 6, 3, 3, 4, 9, 2, 3, 8, 9, 1, 6, 3, 1, 2, 3, 3, 5, 2, 1, 1, 6, 2, 1, 1, 3, 6, 2, 2, 2, 1, 11, 1, 1, 5, 1, 11, 1, 3, 2, 1, 3, 3, 3, 4, 6, 9, 1, 1, 1, 1, 2, 3, 1, 7, 4, 1, 13, 5, 1, 10, 2, 2, 1, 8, 7, 2, 4, 1, 1, 5, 1, 2, 2, 5, 2, 5, 2, 4, 3, 1, 1, 3, 1, 6, 3, 4, 5, 8, 1, 2, 4, 2, 1, 9, 6, 8, 3, 4, 4, 10, 2, 4, 3, 9, 2, 7, 3, 3, 4, 5, 3, 8, 23, 1, 10, 22, 9, 6, 12, 10, 1, 1, 1, 1, 3, 11, 6, 2, 3, 1, 5, 3, 1, 2, 3, 4, 9, 8, 1, 1, 1, 1, 1, 1, 3, 8, 5, 1, 1, 2, 1, 5, 1, 1, 1, 2, 1, 2, 1, 1, 2, 5, 1, 3, 2, 2, 1, 6, 9, 3, 2, 3, 2, 1, 2, 3, 2, 3, 1, 1, 1, 3, 1, 5, 2, 2, 3, 1, 1, 1, 1, 1, 2, 2, 6, 9, 1, 4, 4, 6, 4, 8, 1, 1, 6, 2, 1, 1, 2, 1, 2, 2, 1, 1, 1, 4, 2, 1, 1, 1, 2, 1, 1, 1, 1, 1, 3, 1, 1, 2, 2, 4, 6, 2, 1, 1, 4, 1, 2, 1, 6, 4, 1, 2, 2, 2, 1, 1, 1, 1, 1, 1, 3, 4, 1, 1, 6, 2, 2, 2, 3, 1, 3, 1, 3, 1, 1, 2, 1, 2, 1, 3, 3, 1, 2, 1, 1, 2, 3, 1, 1, 4, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 3, 2, 5, 1, 2, 1, 1, 1, 1, 2, 3, 1, 4, 1, 2, 4, 3, 1, 1, 1, 1, 4, 3, 2, 1, 1, 1, 2, 3, 2, 1, 3, 1, 1, 1, 2, 2, 2, 1, 2, 1, 1, 1, 1, 3, 3, 3, 4, 1, 2, 4, 5, 2, 2, 3, 7, 2, 1, 1, 1, 1, 3, 3, 1, 1, 2, 1, 3, 2, 1, 1, 1, 3, 2, 2, 1, 1, 3, 1, 2, 1, 1, 2, 1, 1, 1, 1, 4, 1, 2, 2, 2, 3, 2, 1, 3, 1, 4, 1, 5, 3, 1, 1, 9, 3, 3, 1, 3, 2, 3, 2, 1, 1, 6, 2, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 3, 1, 3, 3, 3, 4, 3, 1, 4, 1, 1, 1, 4, 1, 5, 1, 4, 2, 1, 1, 3, 1, 2, 1, 3, 2, 2, 2, 1, 1, 2, 3, 2, 2, 2, 2, 1, 1, 10, 5, 2, 1, 1, 2, 1, 3, 3, 2, 1, 1, 2, 3, 1, 2, 1, 1, 1, 3, 2, 1, 1, 2, 3, 2, 4, 2, 4, 5, 1, 5, 1, 3, 1, 6, 2, 2, 3, 1, 3, 8, 4, 3, 1, 3, 12, 1, 4, 2, 1, 3, 6, 1, 6, 2, 4, 1, 2, 3, 3, 2, 3, 1, 4, 2, 1, 2, 2, 1, 3, 1, 1, 1, 4, 1, 1, 2, 2, 4, 4, 2, 2, 1, 1, 2, 3, 4, 1, 2, 1, 2, 1, 1, 2, 1, 1, 1, 4, 3, 2, 3, 2, 5, 1, 2, 1, 4, 3, 6, 4, 1, 1, 11, 2, 1, 6, 1, 1, 1, 1, 2, 2, 1, 1, 3, 2, 6, 1, 8, 4, 2, 4, 3, 4, 3, 1, 2, 1, 3, 2, 2, 7, 1, 2, 2, 2, 4, 2, 2, 4, 4, 2, 2, 1, 3, 1, 1, 14, 5, 3, 1, 2, 10, 2, 3, 3, 7, 2, 1, 6, 8, 1, 3, 3, 3, 3, 1, 1, 1, 3, 7, 3, 1, 2, 9, 4, 8, 3, 6, 2, 4, 5, 1, 2, 2, 4, 13, 14, 14, 3, 2, 7, 6, 5, 8, 2, 2, 1, 5, 1, 2, 4, 1, 1, 1, 1, 5, 4, 1, 4, 4, 2, 1, 1, 2, 1, 3, 1, 14, 1, 1, 1, 1, 2, 2, 4, 1, 1, 1, 1, 1, 1, 3, 1, 1, 1, 5, 2, 5, 9, 1, 2, 1, 3, 5, 2, 1, 1, 1, 2, 1, 1, 2, 5, 3, 2, 4, 12, 1, 6, 3, 2, 1, 1, 1, 1, 1, 2, 5, 5, 9, 1, 1, 5, 15, 4, 1, 2, 2, 11, 3, 2, 6, 1, 1, 1, 1, 1, 5, 4, 1, 5, 2, 3, 7, 6, 5, 3, 2, 4, 1, 3, 3, 1, 5, 3, 4, 5, 4, 4, 5, 5, 1, 4, 1, 5, 4, 2, 1, 4, 6, 1, 5, 1, 1, 6, 1, 11, 2, 1, 10, 7, 3, 11, 2, 11, 2, 2, 1, 3, 1, 4, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 4, 1, 1, 4, 1, 5, 4, 3, 2, 2, 2, 1, 1, 1, 2, 2, 1, 2, 5, 1, 3, 3, 4, 1, 3, 1, 2, 1, 3, 6, 1, 2, 2, 3, 13, 3, 5, 5, 3, 1, 6, 1, 1, 1, 7, 1, 1, 2, 4, 5, 1, 2, 2, 5, 2, 5, 2, 1, 7, 1, 3, 1, 1, 1, 1, 1, 1, 1, 4, 1, 1, 1, 2, 1, 4, 2, 11, 7, 1, 1, 2, 2, 1, 1, 1, 3, 1, 3, 4, 1, 1, 1, 2, 3, 3, 2, 1, 2, 5, 1, 5, 1, 9, 2, 2, 5, 9, 8, 4, 1, 3, 2, 2, 1, 2, 1, 3, 3, 2, 4, 3, 2, 2, 3, 3, 4, 1, 3, 2, 5, 1, 1, 3, 1, 3, 2, 2, 1, 1, 1, 2, 1, 4, 3, 1, 5, 2, 6, 1, 1, 3, 2, 1, 1, 8, 5, 2, 3, 2, 2, 2, 4, 3, 2, 12, 5, 2, 1, 1, 2, 6, 1, 10, 1, 7, 2, 1, 1, 2, 2, 1, 1, 2, 1, 1, 3, 1, 1, 1, 4, 2, 5, 3, 17, 2, 2, 1, 1, 1, 2, 1, 4, 7, 3, 2, 1, 6, 8, 2, 1, 2, 4, 1, 3, 3, 2, 8, 4, 2, 1, 1, 7, 1, 1, 1, 6, 10, 16, 5, 3, 6, 4, 4, 2, 1, 1, 2, 8, 1, 5, 2, 1, 8, 5, 9, 2, 2, 4, 3, 2, 2, 3, 3, 3, 1, 2, 1, 2, 1, 1, 3, 1, 1, 1, 2, 2, 1, 2, 1, 3, 2, 5, 3, 3, 1, 2, 1, 5, 5, 1, 1, 1, 1, 5, 3, 1, 4, 1, 5, 2, 2, 2, 2, 1, 6, 4, 3, 1, 3, 5, 2, 3, 2, 1, 2, 1, 3, 2, 1, 1, 2, 1, 1, 6, 1, 1, 1, 1, 2, 1, 1, 1, 1, 3, 1, 3, 1, 1, 4, 3, 2, 1, 1, 1, 3, 5, 2, 1, 3, 1, 2, 3, 2, 2, 1, 1, 2, 1, 1, 2, 1, 5, 1, 1, 2, 5, 1, 2, 3, 1, 1, 2, 3, 4, 1, 1, 4, 3, 5, 1, 2, 4, 1, 1, 1, 2, 2, 1, 2, 1, 1, 5, 3, 1, 1, 3, 4, 2, 1, 3, 1, 4, 5, 1, 5, 2, 2, 2, 4, 2, 1, 2, 1, 1, 2, 4, 2, 1, 2, 1, 2, 1, 6, 3, 1, 1, 1, 4, 2, 2, 1, 1, 3, 3, 3, 2, 3, 3, 11, 10, 2, 1, 1, 2, 6, 5, 3, 1, 2, 1, 2, 1, 1, 3, 1, 1, 1, 1, 1, 2, 2, 1, 1, 3, 1, 3, 2, 2, 1, 1, 2, 3, 4, 1, 2, 1, 2, 4, 1, 3, 2, 3, 3, 3, 1, 1, 1, 1, 2, 12, 6, 1, 6, 2, 1, 3, 1, 2, 6, 1, 1, 1, 2, 2, 2, 2, 1, 2, 1, 3, 8, 1, 1, 9, 1, 1, 2, 3, 1, 1, 2, 1, 2, 2, 1, 1, 13, 4, 1, 1, 1, 4, 1, 2, 4, 1, 2, 3, 1, 12, 1, 1, 2, 3, 2, 2, 1, 2, 1, 3, 3, 8, 2, 2, 8, 6, 10, 3, 1, 1, 6, 1, 2, 4, 2, 1, 1, 1, 1, 4, 3, 2, 5, 1, 1, 1, 1, 1, 10, 1, 5, 2, 2, 1, 1, 2, 2, 1, 2, 4, 2, 2, 6, 3, 2, 2, 3, 1, 1, 1, 1, 1, 2, 2, 5, 2, 5, 2, 2, 2, 3, 1, 1, 6, 14, 1, 1, 1, 14, 11, 2, 3, 1, 1, 3, 2, 4, 1, 2, 1, 1, 3, 2, 2, 4, 2, 7, 1, 1, 1, 1, 6, 2, 2, 3, 4, 5, 1, 5, 4, 1, 4, 1, 14, 4, 6, 2, 3, 3, 7, 4, 9, 5, 6, 8, 5, 5, 9, 6, 2, 2, 2, 1, 1, 4, 1, 1, 1, 5, 2, 3, 2, 1, 1, 3, 1, 1, 1, 1, 1, 1, 2, 3, 3, 5, 2, 8, 1, 2, 4, 1, 1, 1, 3, 1, 5, 3, 2, 23, 1, 3, 4, 3, 2, 6, 1, 1, 2, 1, 1, 1, 1, 2, 3, 3, 2, 2, 4, 1, 3, 3, 7, 4, 1, 2, 1, 2, 2, 2, 2, 1, 8, 3, 2, 1, 2, 2, 1, 2, 6, 2, 1, 6, 3, 3, 2, 2, 2, 3, 11, 3, 2, 4, 4, 1, 1, 2, 1, 6, 6, 1, 1, 3, 6, 11, 7, 2, 5, 4, 2, 1, 2, 1, 5, 2, 2, 1, 4, 5, 4, 1, 3, 1, 1, 1, 4, 1, 2, 1, 3, 2, 1, 2, 1, 1, 4, 1, 4, 5, 2, 5, 5, 1, 2, 3, 1, 6, 5, 3, 5, 1, 1, 3, 2, 1, 7, 2, 7, 3, 1, 3, 2, 4, 1, 2, 2, 2, 1, 4, 2, 3, 1, 4, 2, 3, 2, 4, 1, 1, 2, 2, 2, 2, 5, 5, 2, 2, 2, 8, 1, 2, 2, 1, 1, 2, 1, 1, 1, 2, 2, 4, 1, 2, 3, 2, 12, 3, 1, 3, 2, 2, 5, 2, 5, 2, 4, 2, 2, 1, 3, 1, 1, 2, 5, 5, 1, 4, 1, 2, 1, 1, 1, 1, 4, 3, 4, 8, 1, 3, 2, 1, 2, 3, 5, 2, 6, 4, 4, 4, 1, 3, 1, 4, 3, 6, 5, 2, 9, 4, 1, 1, 9, 9, 2, 6, 3, 1, 12, 1, 5, 1, 1, 1, 1, 2, 9, 1, 18, 1, 4, 8, 6, 1, 1, 7, 1, 1, 12, 8, 1, 3, 2, 1, 4, 1, 1, 1, 5, 1, 5, 4, 3, 1, 1, 6, 3, 1, 1, 1, 2, 4, 5, 2, 1, 8, 1, 1, 2, 3, 2, 1, 1, 7, 1, 7, 12, 3, 2, 4, 1, 1, 2, 1, 1, 1, 2, 3, 3, 1, 1, 2, 2, 1, 1, 1, 4, 1, 1, 5, 4, 3, 1, 1, 3, 6, 9, 2, 2, 10, 5, 6, 3, 3, 1, 4, 2, 6, 3, 1, 15, 18, 9, 4, 2, 4, 7, 1, 3, 1, 2, 1, 5, 3, 2, 11, 1, 7, 6, 2, 3, 1, 2, 3, 5, 1, 6, 5, 7, 32, 2, 1, 1, 1, 11, 3, 3, 10, 2, 1, 3, 2, 3, 1, 13, 1, 5, 4, 3, 8, 1, 1, 2, 6, 6, 3, 1, 5, 10, 3, 7, 5, 1, 4, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 2, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 2, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 3, 5, 2, 1, 3, 8, 1, 3, 1, 2, 1, 2, 3, 2, 2, 1, 6, 1, 1, 4, 2, 1, 1, 2, 1, 2, 2, 4, 1, 3, 4, 3, 2, 2, 2, 3, 3, 6, 4, 1, 2, 3, 3, 1, 1, 6, 1, 2, 6, 1, 3, 1, 2, 2, 4, 5, 2, 2, 4, 2, 1, 3, 2, 5, 1, 1, 4, 4, 5, 2, 1, 2, 6, 2, 1, 4, 1, 2, 1, 1, 1, 5, 1, 1, 3, 2, 3, 1, 1, 1, 1, 3, 4, 2, 3, 1, 1, 1, 1, 1, 7, 2, 3, 1, 1, 3, 2, 2, 3, 1, 3, 4, 1, 1, 6, 2, 2, 2, 2, 4, 11, 1, 5, 1, 1, 1, 1, 2, 2, 6, 1, 1, 4, 1, 1, 1, 1, 2, 2, 1, 7, 3, 2, 2, 1, 2, 2, 1, 1, 1, 2, 1, 4, 1, 1, 3, 1, 2, 1, 2, 6, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 6, 1, 1, 8, 1, 2, 1, 1, 1, 1, 1, 3, 3, 1, 2, 1, 2, 1, 1, 1, 2, 1, 1, 2, 1, 2, 2, 2, 2, 1, 5, 3, 1, 2, 1, 1, 3, 2, 1, 1, 1, 1, 1, 2, 2, 2, 9, 8, 1, 3, 2, 1, 6, 1, 5, 8, 2, 2, 1, 1, 2, 2, 1, 2, 1, 1, 5, 6, 5, 1, 1, 2, 2, 2, 1, 2, 1, 2, 1, 4, 1, 1, 1, 1, 2, 1, 1, 2, 6, 2, 5, 2, 7, 1, 1, 1, 1, 1, 3, 6, 2, 1, 10, 7, 1, 2, 2, 1, 2, 1, 3, 3, 1, 2, 1, 11, 4, 3, 4, 2, 2, 1, 2, 2, 1, 6, 1, 6, 1, 1, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 2, 1, 3, 2, 2, 2, 5, 1, 2, 1, 3, 8, 1, 1, 4, 1, 3, 1, 1, 1, 1, 1, 2, 9, 3, 4, 13, 1, 6, 7, 2, 6, 1, 1, 1, 1, 1, 4, 3, 3, 1, 1, 3, 2, 6, 1, 2, 1, 3, 1, 5, 2, 3, 1, 2, 2, 4, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 2, 4, 1, 1, 2, 1, 1, 1, 3, 3, 3, 1, 3, 1, 1, 1, 1, 2, 1, 1, 1, 1, 2, 1, 3, 1, 2, 1, 2, 2, 1, 1, 1, 5, 4, 2, 3, 3, 2, 2, 5, 1, 1, 1, 1, 1, 1, 3, 3, 2, 1, 2, 2, 1, 1, 2, 3, 2, 11, 1, 3, 1, 1, 2, 2, 1, 2, 1, 1, 2, 1, 3, 4, 3, 3, 1, 2, 1, 5, 1, 1, 1, 2, 6, 2, 2, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 8, 1, 3, 1, 1, 8, 1, 3, 1, 4, 4, 1, 3, 2, 3, 1, 2, 2, 2, 6, 1, 1, 1, 6, 1, 1, 1, 3, 1, 1, 1, 2, 1, 3, 1, 4, 2, 3, 2, 3, 5, 2, 2, 4, 3, 1, 1, 8, 1, 5, 1, 1, 1, 6, 1, 4, 4, 2, 1, 6, 1, 1, 1, 1, 1, 2, 7, 4, 1, 7, 1, 1, 1, 1, 4, 5, 4, 3, 4, 1, 9, 3, 8, 6, 3, 1, 6, 2, 6, 2, 1, 1, 4, 2, 2, 1, 1, 1, 2, 8, 9, 1, 4, 7, 2, 3, 2, 1, 2, 14, 3, 2, 4, 1, 1, 2, 1, 2, 2, 3, 5, 1, 1, 2, 3, 1, 2, 3, 7, 2, 1, 5, 1, 6, 2, 14, 3, 18, 2, 1, 3, 3, 5, 2, 4, 6, 1, 11, 1, 2, 1, 1, 1, 4, 2, 5, 1, 1, 12, 3, 5, 6, 4, 1, 1, 3, 5, 2, 6, 1, 3, 6, 1, 2, 1, 7, 2, 10, 1, 8, 2, 2, 1, 1, 4, 1, 1, 7, 2, 8, 8, 4, 2, 1, 3, 2, 7, 1, 3, 4, 1, 4, 16, 2, 4, 6, 1, 1, 3, 1, 6, 5, 4, 13, 2, 4, 11, 7, 15, 1, 1, 1, 3, 1, 5, 1, 1, 2, 1, 3, 5, 4, 1, 2, 3, 8, 1, 1, 1, 1, 1, 1, 1, 1, 1, 8, 1, 1, 1, 6, 2, 7, 1, 6, 1, 3, 1, 1, 1, 2, 3, 1, 1, 3, 2, 4, 6, 4, 2, 1, 2, 1, 1, 3, 1, 1, 4, 4, 1, 2, 3, 1, 5, 5, 4, 1, 2, 1, 1, 1, 1, 4, 1, 2, 3, 2, 2, 1, 4, 1, 1, 1, 3, 5, 1, 2, 2, 1, 2, 2, 2, 3, 1, 1, 2, 4, 5, 3, 3, 3, 4, 4, 8, 1, 1, 3, 2, 5, 2, 2, 2, 6, 1, 3, 2, 1, 2, 6, 2, 4, 2, 2, 3, 1, 1, 1, 4, 1, 2, 3, 3, 2, 18, 1, 2, 6, 3, 1, 1, 2, 4, 2, 5, 3, 3, 6, 4, 1, 6, 1, 9, 5, 5, 5, 2, 2, 4, 7, 2, 3, 7, 5, 3, 6, 2, 1, 9, 2, 1, 1, 14, 1, 7, 2, 1, 4, 4, 1, 7, 1, 6, 1, 3, 2, 2, 2, 4, 1, 1, 2, 3, 1, 2, 1, 2, 2, 2, 5, 6, 1, 1, 2, 1, 2, 1, 3, 3, 2, 5, 4, 6, 6, 2, 7, 2, 4, 6, 10, 1, 3, 1, 3, 5, 1, 3, 2, 1, 3, 1, 3, 1, 2, 2, 1, 2, 1, 3, 8, 1, 2, 1, 4, 1, 4, 1, 4, 5, 3, 6, 2, 1, 6, 1, 1, 4, 2, 4, 5, 1, 5, 2, 2, 2, 5, 6, 3, 26, 1, 1, 6, 4, 5, 3, 9, 4, 2, 2, 3, 5, 4, 4, 3, 16, 4, 2, 11, 3, 3, 3, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 3, 1, 1, 1, 1, 2, 6, 4, 5, 2, 1, 5, 4, 4, 5, 7, 1, 2, 1, 4, 1, 2, 2, 2, 5, 2, 2, 1, 2, 3, 2, 4, 7, 3, 4, 3, 6, 1, 3, 8, 2, 8, 5, 6, 3, 3, 1, 4, 3, 2, 2, 1, 3, 1, 1, 6, 3, 11, 2, 1, 2, 1, 6, 2, 3, 6, 2, 1, 3, 2, 2, 2, 6, 2, 7, 3, 3, 8, 2, 2, 7, 4, 5, 1, 10, 1, 7, 3, 6, 1, 2, 4, 2, 2, 5, 3, 4, 2, 7, 2, 2, 2, 3, 12, 13, 5, 11, 6, 9, 2, 2, 10, 6, 3, 1, 4, 3, 6, 3, 10, 9, 7, 7, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 10, 5, 1, 1, 1, 3, 2, 2, 4, 1, 5, 7, 1, 6, 2, 1, 1, 7, 5, 1, 1, 8, 3, 4, 1, 2, 1, 6, 1, 4, 5, 1, 2, 1, 1, 5, 1, 1, 1, 1, 1, 6, 2, 2, 1, 1, 3, 1, 3, 1, 1, 1, 1, 1, 3, 1, 2, 8, 1, 9, 2, 2, 3, 1, 2, 4, 2, 1, 7, 6, 2, 1, 8, 3, 2, 3, 2, 4, 3, 7, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 4, 2, 3, 3, 1, 1, 1, 1, 10, 3, 1, 5, 1, 6, 3, 1, 4, 6, 4, 5, 9, 2, 1, 1, 3, 2, 4, 1, 4, 1, 2, 5, 1, 2, 1, 1, 1, 5, 1, 3, 1, 2, 1, 1, 2, 2, 1, 4, 1, 1, 2, 3, 4, 2, 1, 1, 1, 1, 1, 1, 10, 4, 1, 2, 3, 1, 2, 2, 9, 1, 1, 3, 7, 1, 2, 2, 1, 1, 1, 3, 1, 4, 1, 3, 3, 1, 1, 2, 3, 1, 1, 1, 1, 1, 5, 1, 7, 1, 5, 5, 2, 3, 3, 1, 3, 2, 2, 2, 4, 2, 1, 1, 1, 2, 4, 2, 3, 1, 2, 2, 1, 12, 2, 3, 9, 1, 1, 3, 3, 8, 6, 10, 1, 1, 1, 5, 13, 25, 11, 6, 1, 9, 1, 1, 6, 2, 2, 10, 1, 5, 9, 3, 3, 1, 3, 2, 5, 4, 1, 17, 7, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 3, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 4, 3, 2, 1, 2, 2, 1, 1, 1, 1, 1, 7, 4, 1, 3, 1, 2, 1, 2, 1, 6, 2, 1, 2, 1, 1, 3, 1, 2, 1, 1, 1, 1, 3, 2, 1, 1, 2, 1, 1, 1, 2, 1, 2, 4, 3, 1, 1, 4, 3, 1, 1, 1, 1, 1, 1, 3, 1, 1, 2, 2, 1, 1, 1, 2, 2, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 3, 8, 1, 2, 1, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3, 3, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 1, 2, 2, 2, 1, 2, 3, 1, 4, 1, 4, 2, 1, 2, 1, 2, 2, 1, 2, 1, 1, 1, 1, 1, 4, 2, 1, 4, 2, 2, 2, 2, 2, 2, 2, 5, 1, 3, 4, 2, 2, 1, 1, 1, 2, 1, 1, 1, 2, 1, 1, 1, 3, 3, 3, 1, 2, 3, 4, 1, 2, 2, 4, 4, 1, 1, 4, 1, 2, 2, 1, 1, 3, 2, 2, 5, 1, 3, 5, 1, 1, 3, 2, 5, 16, 5, 4, 5, 6, 2, 1, 1, 18, 3, 3, 1, 1, 1, 1, 3, 2, 5, 3, 3, 1, 3, 1, 1, 1, 1, 4, 1, 2, 2, 1, 1, 1, 4, 1, 1, 1, 1, 4, 2, 4, 2, 2, 2, 2, 1, 1, 3, 2, 8, 1, 1, 2, 4, 1, 7, 1, 1, 2, 1, 5, 6, 6, 1, 6, 2, 1, 1, 1, 1, 1, 1, 1, 1, 7, 4, 1, 6, 3, 35, 4, 7, 1, 9, 8, 14, 3, 6, 1, 7, 12, 6, 3, 1, 9, 16, 4, 3, 5, 5, 19, 4, 16, 2, 5, 4, 2, 8, 2, 3, 2, 4, 6, 24, 6, 8, 4, 8, 12, 8, 10, 4, 2, 3, 3, 1, 7, 8, 12, 1, 2, 14, 15, 11, 9, 4, 13, 5, 2, 1, 3, 20, 26, 2, 12, 2, 7, 2, 1, 2, 25, 27, 22, 4, 3, 3, 2, 4, 6, 13, 13, 5, 8, 5, 2, 10, 1, 2, 8, 1, 1, 1, 1, 1, 1, 1, 1, 3, 1, 2, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 3, 1, 1, 1, 1, 1, 1, 1, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 1, 1, 5, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 3, 1, 1, 1, 2, 1, 2, 2, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1, 4, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 2, 5, 1, 1, 5, 2, 1, 2, 1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 2, 1, 1, 1, 3, 1, 2, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 8, 1, 3, 6, 2, 6, 1, 1, 5, 9, 2, 2, 3, 9, 8, 2, 3, 8, 1, 3, 2, 8, 2, 6, 6, 6, 1, 1, 3, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 1, 2, 1, 2, 3, 2, 9, 4, 3, 1, 1, 1, 1, 1, 5, 4, 1, 2, 2, 1, 1, 1, 1, 1, 2, 1, 1, 3, 4, 1, 6, 4, 2, 1, 1, 1, 4, 1, 1, 2, 4, 2, 1, 2, 1, 1, 1, 5, 8, 1, 2, 2, 1, 2, 1, 1, 1, 4, 1, 2, 1, 1, 2, 1, 11, 1, 2, 6, 1, 2, 3, 2, 3, 1, 1, 1, 2, 1, 3, 1, 1, 1, 2, 3, 1, 1, 1, 3, 3, 1, 3, 3, 2, 2, 2, 1, 5, 1, 1, 5, 2, 1, 1, 3, 1, 2, 2, 3, 2, 1, 3, 2, 1, 1, 1, 4, 1, 1, 2, 2, 3, 6, 2, 9, 1, 2, 3, 3, 2, 6, 1, 5, 4, 6, 10, 1, 3, 3, 2, 1, 2, 2, 1, 1, 3, 4, 7, 3, 1, 2, 2, 2, 2, 3, 6, 6, 2, 2, 2, 1, 6, 8, 1, 2, 3, 1, 1, 3, 1, 1, 2, 3, 2, 13, 5, 5, 1, 7, 19, 1, 2, 1, 1, 1, 1, 6, 2, 1, 5, 4, 2, 1, 3, 1, 2, 4, 4, 1, 1, 1, 4, 1, 12, 9, 3, 8, 1, 2, 11, 6, 1, 1, 1, 9, 3, 3, 9, 4, 4, 2, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 11, 3, 14, 2, 8, 3, 1, 1, 2, 3, 1, 2, 3, 1, 3, 5, 1, 2, 5, 3, 2, 8, 1, 1, 5, 2, 5, 2, 1, 3, 2, 1, 5, 9, 7, 4, 2, 7, 8, 2, 6, 4, 2, 1, 5, 1, 2, 1, 2, 7, 7, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 1, 2, 2, 2, 1, 1, 1, 1, 1, 2, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 12, 3, 4, 1, 1, 3, 2, 1, 13, 15, 2, 3, 6, 2, 16, 4, 14, 2, 13, 8, 3, 23, 7, 7, 7, 16, 2, 3, 5, 3, 10, 1, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 3, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 7, 1, 6, 1, 1, 2, 2, 4, 1, 1, 3, 6, 1, 3, 2, 1, 1, 1, 2, 2, 9, 5, 5, 2, 1, 1, 10, 2, 8, 3, 2, 5, 2, 4, 7, 5, 3, 1, 2, 2, 10, 1, 9, 1, 5, 1, 1, 1, 1, 1, 1, 1, 1, 4, 2, 2, 3, 4, 2, 5, 16, 8, 26, 25, 3, 1, 27, 1, 23, 7, 8, 35, 26, 5, 23, 9, 5, 32, 5, 3, 1, 1, 1, 3, 1, 1, 1, 3, 2, 1, 2, 1, 3, 6, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 3, 3, 1, 1, 1, 1, 1, 1, 1, 3, 1, 1, 3, 3, 1, 1, 2, 3, 2, 1, 1, 1, 1, 3, 1, 1, 1, 1, 2, 3, 1, 1, 1, 3, 3, 4, 10, 1, 21, 18, 11, 2, 19, 4, 30, 4, 15, 2, 61, 5, 11, 24, 27, 7, 28, 5, 5, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 1, 1, 4, 2, 1, 2, 1, 1, 2, 6, 2, 1, 2, 1, 1, 1, 1, 1, 2, 3, 6, 3, 1, 2, 3, 5, 1, 3, 7, 1, 4, 6, 2, 6, 1, 15, 3, 3, 1, 1, 1, 5, 1, 9, 1, 1, 2, 3, 4, 3, 1, 1, 1, 1, 1, 2, 3, 2, 1, 1, 1, 5, 12, 1, 2, 13, 2, 1, 2, 3, 6, 7, 2, 10, 3, 10, 2, 3, 3, 6, 2, 1, 1, 5, 1, 1, 15, 2, 3, 4, 13, 8, 1, 3, 1, 1, 2, 1, 1, 1, 1, 1, 1, 3, 9, 1, 1, 1, 3, 1};
		static ImWchar base_ranges[] = // not zero-terminated
		    {
		        0x0020,
		        0x00FF, // Basic Latin + Latin Supplement
		        0x3000,
		        0x30FF, // Punctuations, Hiragana, Katakana
		        0x31F0,
		        0x31FF, // Katakana Phonetic Extensions
		        0xFF00,
		        0xFFEF, // Half-width characters
		    };
		static ImWchar full_ranges[IM_ARRAYSIZE(base_ranges) + IM_ARRAYSIZE(accumulative_offsets_from_0x4E00) * 2 + 1] = {0};
		if (!full_ranges[0])
		{
			memcpy(full_ranges, base_ranges, sizeof(base_ranges));
			ImWchar* out_ranges = full_ranges + IM_ARRAYSIZE(base_ranges);
			int base_codepoint  = 0x4E00;
			for (int n = 0; n < IM_ARRAYSIZE(accumulative_offsets_from_0x4E00); n++, out_ranges += 2)
			{
				out_ranges[0] = out_ranges[1] = (ImWchar)(base_codepoint + accumulative_offsets_from_0x4E00[n]);
				base_codepoint += accumulative_offsets_from_0x4E00[n];
			}
			out_ranges[0] = 0;
		}
		return &full_ranges[0];
	}

	const ImWchar* font_mgr::GetGlyphRangesTurkish()
	{
		static const ImWchar icons_ranges_Turkish[] = {
		    0x0020,
		    0x00FF, // Basic Latin + Latin Supplement
		    0x00c7,
		    0x00c7, // Ç
		    0x00e7,
		    0x00e7, // ç
		    0x011e,
		    0x011e, // Ğ
		    0x011f,
		    0x011f, // ğ
		    0x0130,
		    0x0130, // İ
		    0x0131,
		    0x0131, // ı
		    0x00d6,
		    0x00d6, // Ö
		    0x00f6,
		    0x00f6, // ö
		    0x015e,
		    0x015e, // Ş
		    0x015f,
		    0x015f, // ş
		    0x00dc,
		    0x00dc, // Ü
		    0x00fc,
		    0x00fc, // ü
		    0,
		};
		return &icons_ranges_Turkish[0];
	}

	const ImWchar* font_mgr::get_imgui_alphabet_type()
	{
		auto& io = ImGui::GetIO();
		switch (m_require_extra)
		{
		case eAlphabetType::CHINESE: return GetGlyphRangesChineseSimplifiedOfficial();
		case eAlphabetType::CYRILLIC: return io.Fonts->GetGlyphRangesCyrillic();
		case eAlphabetType::JAPANESE: return io.Fonts->GetGlyphRangesJapanese();
		case eAlphabetType::KOREAN: return io.Fonts->GetGlyphRangesKorean();
		case eAlphabetType::TURKISH: return GetGlyphRangesTurkish();

		default:
		case eAlphabetType::LATIN: return io.Fonts->GetGlyphRangesDefault();
		}
	}
}
