#pragma once

namespace big
{
	struct WeatherType
	{
		const char* nativename;
		const char* translatedname;
	};

	static constexpr inline auto weathers = std::to_array<WeatherType>({
	    {"EXTRASUNNY", "格外晴朗"},
	    {"CLEAR", "晴朗"},
	    {"CLOUDS", "多云"},
	    {"SMOG", "阴霾"},
	    {"FOGGY", "大雾"},
	    {"OVERCAST", "阴天"},
	    {"RAIN", "雨"},
	    {"THUNDER", "雷雨"},
	    {"CLEARING", "雨转晴"},
	    {"NEUTRAL", "阴雨"},
	    {"SNOW", "雪"},
	    {"BLIZZARD", "暴雪"},
	    {"SNOWLIGHT", "小雪"},
	    {"XMAS", "圣诞节"},
	    {"HALLOWEEN", "万圣节"},
	    {"SNOW_HALLOWEEN", "万圣节-雪"},
	    {"RAIN_HALLOWEEN", "万圣节-雨"},
	});
}