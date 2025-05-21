#include "logger.hpp"
using namespace std::chrono_literals;

my_log::log_format_setting& first_init()
{
	auto& setting = my_log::log_format_setting::getInstance();
	return setting;
}

int main()
{
	// auto& setting = first_init();
	my_log::sync_logger logger("test", my_log::log_level::value::DEBUG);
	std::vector<my_log::output_t> output_arr;
	output_arr.emplace_back(my_log::output_t{ my_log::screen_output{} });
	logger.addFmtStrAndOutput("default", std::move(output_arr));
	output_arr.emplace_back(my_log::output_t{ my_log::screen_output{} });
	logger.addFmtStrAndOutput("highlight", std::move(output_arr));
	// logger.setStartupDuration(1h);
	logger.start();
	for (size_t i = 0; i < 100; ++i)
	{
		logger.debug(std::source_location::current(), std::format("hello world {0:d}", i));
	}
	return 0;
}