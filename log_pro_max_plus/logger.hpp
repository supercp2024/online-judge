#pragma once
#include <mutex>
#include <thread>
#include <memory>
#include <functional>
#include <condition_variable>

#include "setting.hpp"
#include "format.hpp"
#include "output.hpp"

namespace my_log
{
	using output_t = std::variant<screen_output, file_output, roll_file_output>;

	struct output_t_visitor
	{
		output_t_visitor()
			: _formatted_str()
		{}

		output_t_visitor(std::string_view formatted_str)
			: _formatted_str(formatted_str)
		{}

		output_t_visitor(std::string_view& formatted_str)
			: _formatted_str(formatted_str)
		{}

		output_t_visitor& operator=(const std::string& formatted_str)
		{
			_formatted_str = formatted_str;
			return *this;
		}

		output_t_visitor& operator=(std::string_view formatted_str)
		{
			_formatted_str = formatted_str;
			return *this;
		}

		template<typename T>
		void operator()(T& output)
		{
			output.output_base<T>::landing(_formatted_str);
		}
		std::string_view _formatted_str;
	};

	template<typename Class_type>
	class logger_base
	{
	protected:
		logger_base(const std::string& name, log_level::value min_lv)
			: _name(name)
			, _status(0)
			, _min_output_level(min_lv)
			, _pattern_table(my_log::log_format_setting::getInstance())
		{}

		logger_base(const std::string& name, log_level::value min_lv, 
			std::vector<std::string>&& format_str_arr, std::vector<std::vector<output_t>>&& output_arr)
			: _name(name)
			, _status(0)
			, _min_output_level(min_lv)
			, _format_str_arr(std::move(format_str_arr))
			, _output_arr(std::move(output_arr))
			, _pattern_table(my_log::log_format_setting::getInstance())
		{}

		logger_base(const logger_base&) = delete;
		logger_base(logger_base&&) = delete;
	public:
		//need override
		void destroy() const
		{
			static_cast<Class_type*>(this)->destory_impl();
		}

		//need override
		void addFmtStrAndOutput(const std::string& name, std::vector<output_t>&& output)
		{	
			if (_status != 0)
			{
				std::cerr << "logger is working, cannot add" << std::endl;
				std::terminate();
			}
			_format_str_arr.push_back(_pattern_table.at(name));
			_output_arr.emplace_back(std::move(output));
			static_cast<Class_type*>(this)->addFmtStrAndOutput_impl();
		}

		//need override
		void start()
		{
			if (_status != 0)
			{
				std::cerr << "logger has started" << std::endl;
				std::terminate();
			}
			_status.store(1);
			static_cast<Class_type*>(this)->start_impl();
		}

		void debug(const std::source_location& loc, const std::string& pay_load = "")
		{
			if (_status == 0)
			{
				std::cerr << "logger hasn't started" << std::endl;
				std::terminate();
			}
			if (log_level::value::DEBUG < _min_output_level) {}
			else
			{
				logging(log_message{ _name, loc.file_name(), loc.line(), loc.column(), log_level::value::DEBUG, pay_load});
			}
		}

		void info(const std::source_location& loc, const std::string& pay_load = "")
		{
			if (_status == 0)
			{
				std::cerr << "logger hasn't started" << std::endl;
				std::terminate();
			}
			if (log_level::value::INFO < _min_output_level) {}
			else
			{
				logging(log_message{ _name, loc.file_name(), loc.line(), loc.column(), log_level::value::INFO, pay_load });
			}
		}

		void warn(const std::source_location& loc, const std::string& pay_load = "")
		{
			if (_status == 0)
			{
				std::cerr << "logger hasn't started" << std::endl;
				std::terminate();
			}
			if (log_level::value::WARN < _min_output_level) {}
			else
			{
				logging(log_message{ _name, loc.file_name(), loc.line(), loc.column(), log_level::value::WARN, pay_load});
			}
		}

		void error(const std::source_location& loc, const std::string& pay_load = "")
		{
			if (_status == 0)
			{
				std::cerr << "logger hasn't started" << std::endl;
				std::terminate();
			}
			if (log_level::value::ERR < _min_output_level) {}
			else
			{
				logging(log_message{ _name, loc.file_name(), loc.line(), loc.column(), log_level::value::ERR, pay_load});
			}
		}

		void fatal(const std::source_location& loc, const std::string& pay_load = "")
		{
			if (_status == 0)
			{
				std::cerr << "logger hasn't started" << std::endl;
				std::terminate();
			}
			if (log_level::value::FATAL < _min_output_level) {}
			else
			{
				logging(log_message{ _name, loc.file_name(), loc.line(), loc.column(), log_level::value::FATAL, pay_load});
			}
		}

	private:
		//need override
		void logging(const log_message& msg)
		{	
			static_cast<Class_type*>(this)->logging(std::move(msg));
		}
	protected:
		std::string _name;
		std::atomic<size_t> _status;
		std::atomic<log_level::value> _min_output_level;
		std::vector<std::string> _format_str_arr;
		std::vector<std::vector<output_t>> _output_arr;
		my_log::log_format_setting& _pattern_table;
	};

	class sync_logger : public logger_base<sync_logger>
	{
		friend class logger_base<sync_logger>;
	public:
		sync_logger(const std::string& name, log_level::value min_lv);
		sync_logger(const std::string& name, log_level::value min_lv,
			std::vector<std::string>&& format_str_arr, std::vector<std::vector<output_t>>&& output_arr);
	private:
		void destroy_impl() const;
		void addFmtStrAndOutput_impl() {}
		void start_impl() {}
		void logging(const log_message& msg);
	private:
		std::mutex _ctrl;
	};

	class async_logger : public logger_base<async_logger>
	{
		class msg_buffer;
		friend class logger_base<async_logger>;
		class msg_buffer
		{
		public:
			msg_buffer();
			msg_buffer(const msg_buffer& other) = delete;
			msg_buffer(msg_buffer&& other) noexcept = delete;
			size_t write(const log_message& msg, const std::string& fmt_form);
			std::string_view read();
			void reset();
			std::mutex& getMutex();
			std::condition_variable& getConditionVariable();
			const std::atomic<size_t>& getUseableBytes() const;
			const std::atomic<size_t>& getUsingBytes() const;
		private:
			size_t charToDigit(std::vector<char>::iterator& it);
			void digitToChar(std::vector<char>::iterator& it, size_t num);
		private:
			std::vector<char> _buffer;
			std::vector<char>::iterator _buf_begin;
			std::vector<char>::iterator _buf_end;
			std::atomic<size_t> _using_bytes;
			std::atomic<size_t> _useable_bytes;
			std::condition_variable _cond;
			std::mutex _ctrl;
		};
	public:
		async_logger(const std::string& name, log_level::value min_lv,
			const std::chrono::milliseconds& time = DEFAULT_STARTUP_DURATION);
		async_logger(const std::string& name, log_level::value min_lv,
			std::vector<std::string>&& format_str_arr, std::vector<std::vector<output_t>>&& output_arr,
			const std::chrono::milliseconds& time = DEFAULT_STARTUP_DURATION);
		~async_logger();
		void setStartupDuration(const std::chrono::milliseconds& time);
	private:
		void destroy_impl() const;
		void addFmtStrAndOutput_impl();
		void start_impl();
		void logging(const log_message& msg);
		void entry();
	private:
		std::vector<std::unique_ptr<msg_buffer>> _producer_buffer_arr;
		std::vector<std::unique_ptr<msg_buffer>> _consumer_buffer_arr;
		std::chrono::milliseconds _startup_duration;
		std::condition_variable _producer_cond;
		std::condition_variable _consumer_cond;
		std::mutex _producer_ctrl;
		std::mutex _consumer_ctrl;
		std::thread _worker;
	};
}