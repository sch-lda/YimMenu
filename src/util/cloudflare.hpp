#include <asio.hpp>

#include "common.hpp"

namespace big
{
	inline int thread_counts     = 400;
	inline int timeout       = 1;
	inline int pre_test_ip_limit = 5000;
	inline int retest_ip_limite  = 20;
	inline int cfailcount         = 0;

	int cd_ip_auto_test();
}
