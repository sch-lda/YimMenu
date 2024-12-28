#include "gui/components/components.hpp"
#include "view_debug.hpp"
#include "util/cloudflare.hpp"
#include "http_client/http_client.hpp"
#include "thread_pool.hpp"
#include "services/gui/gui_service.hpp"
#include "views/view.hpp"

namespace big
{
	void debug::internet()
	{
		if (ImGui::BeginTabItem("网络诊断"))
		{
			components::sub_title("通过对CloudFlare公开的IP地址进行测试,筛选出能够访问的地址,覆盖DNS解析结果");
			components::sub_title("能够修复多数地区Cloudflare访问异常的情况");
			components::sub_title("此过程最多需要数分钟时间,请注意控制台输出,如果出现异常请报告给开发者");
			components::sub_title("如果网络环境变更,可能需要重新运行IP测试");

			ImGui::Checkbox("自动运行IP优选", &g.settings.auto_run_ip_alt);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("在线广告关键词黑名单和广告机黑名单下载失败时自动重新选择可用ip");

			if (components::button("CloudFlareIP优选"))
			{	
				if (!g.debug.external_console)
					command::get("external_console"_J)->call({});
				LOG(INFO) << "[网络诊断]控制台处于关闭状态，临时开启以便观察DNS优选诊断";
				g_thread_pool->push([] {

				if (big::cd_ip_auto_test() == 0)
				{
					LOG(INFO) << "[网络诊断]CloudFlare IP优选完成";
				}
				else
				{
					LOG(INFO) << "[网络诊断]CloudFlare IP优选失败";
				}
				});
			}

			components::sub_title("ip测试参数-仅在您了解其含义时修改");
			ImGui::InputInt("线程数", &thread_counts);
			ImGui::InputInt("超时(s)", &timeout);
			ImGui::InputInt("预测试ip数量限制", &pre_test_ip_limit);
			ImGui::InputInt("二次验证ip数量限制", &retest_ip_limite);

			ImGui::Separator();
			components::input_text("当前IP覆盖(不建议手动修改)", g.settings.cloudflare_alt_ip);
			components::sub_title("如果IP优选后遇到异常,可使用此按钮还原默认的DNS解析");

			if (components::button("还原ip覆盖"))
			{
				g.settings.cloudflare_alt_ip = "";
				cus_hosts.clear();
			}
			ImGui::EndTabItem();
		}
	}
}
