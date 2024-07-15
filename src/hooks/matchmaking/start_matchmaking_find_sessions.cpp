#include "fiber_pool.hpp"
#include "function_types.hpp"
#include "hooking/hooking.hpp"
#include "services/matchmaking/matchmaking_service.hpp"
#include "services/player_database/player_database_service.hpp"
#include "util/chat.hpp"

#include <network/Network.hpp>

namespace big
{
	bool hooks::start_matchmaking_find_sessions(int profile_index, int available_slots, NetworkGameFilterMatchmakingComponent* filter, unsigned int max_sessions, rage::rlSessionInfo* results, int* num_sessions_found, rage::rlTaskStatus* status)
	{
		int discriminator = filter->m_param_values[0];// this is guaranteed to work

		if (g.session_browser.replace_game_matchmaking && filter->m_filter_type == 1)
		{
			status->status = 1;
			g_fiber_pool->queue_job([max_sessions, results, num_sessions_found, status, discriminator] {
				bool result = false;

				result = g_matchmaking_service->matchmake(discriminator, !g.session.join_in_sctv_slots);

				if (result)
				{
					for (int i = 0; i < g_matchmaking_service->get_num_found_sessions(); i++)
					{
						if (g_matchmaking_service->get_found_sessions()[i].is_valid)
						{
							int is_host_spam = 0;

							auto host_rid =
							    g_matchmaking_service->get_found_sessions()[i].info.m_net_player_data.m_gamer_handle.m_rockstar_id;
							auto player = g_player_database_service->get_player_by_rockstar_id(host_rid);
							auto multiplex_count = g_matchmaking_service->get_found_sessions()[i].attributes.multiplex_count;
							
							if ((g.session_browser.exclude_modder_sessions && player && player->block_join) ||
							        (g.session_browser.filter_multiplexed_sessions && multiplex_count > 1))
										continue;

							if (g.session_browser.exclude_ad_sessions)
							{
								for (auto rid : spam_rid)
								{
									if (rid == host_rid)
										is_host_spam = 1;
								}
							}

							if (is_host_spam == 1)
								continue;

							results[*num_sessions_found] = g_matchmaking_service->get_found_sessions()[i].info;
							(*num_sessions_found)++;

							if (max_sessions <= *num_sessions_found)
								break;
						}
					}

					status->status = 3;
				}
				else
				{
					status->status = 2;
				}
			});
			return true;
		}
		else
		{
			return g_hooking->get_original<hooks::start_matchmaking_find_sessions>()(profile_index, available_slots, filter, max_sessions, results, num_sessions_found, status);
		}
	}
}