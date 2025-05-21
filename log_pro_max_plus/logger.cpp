#include "logger.hpp"
#include <cstring>

namespace my_log
{
	sync_logger::sync_logger(const std::string& name, log_level::value min_lv)
		: logger_base(name, min_lv)
		, _ctrl()
	{}

	sync_logger::sync_logger(const std::string& name, log_level::value min_lv,
		std::vector<std::string>&& format_str_arr, std::vector<std::vector<output_t>>&& output_arr)
		: logger_base(name, min_lv, std::move(format_str_arr), std::move(output_arr))
		, _ctrl()
	{}

	void sync_logger::destroy_impl() const
	{
		delete this;
	}

	void sync_logger::logging(const log_message& msg)
	{
		std::lock_guard lock(_ctrl);
		std::string formatted_str;
		output_t_visitor visitor(formatted_str);
		for (size_t i = 0; i < _format_str_arr.size(); ++i)
		{
			visitor = formatted_str = std::vformat(_format_str_arr[i], std::make_format_args(msg));
			for (auto& output : _output_arr[i])
			{
				std::visit(visitor, output);
			}
		}
	}

	async_logger::msg_buffer::msg_buffer()
		: _buffer(MAX_BUFFER_SIZE, '\0')
		, _buf_begin(_buffer.begin())
		, _buf_end(_buffer.begin())
		, _using_bytes(0)
		, _useable_bytes(MAX_BUFFER_SIZE)
		, _cond()
		, _ctrl()
	{}

	size_t async_logger::msg_buffer::write(const log_message& msg, const std::string& fmt_form)
	{
		std::unique_lock lock(_ctrl, std::defer_lock_t{});
		if (lock.try_lock())
		{
			if (_useable_bytes.load() < MAX_FORMATTED_STR_LEN)
			{
				lock.unlock();
				_cond.notify_one();
				return 1;
			}
			std::vector<char>::iterator num_it = _buf_end;
			_buf_end += sizeof(size_t);
			_buf_end = std::vformat_to(_buf_end, fmt_form, std::make_format_args(msg));
			digitToChar(num_it, _buf_end - num_it - sizeof(size_t));
			_useable_bytes.fetch_sub(_buf_end - num_it);
			_using_bytes.fetch_add(_buf_end - num_it);
			lock.unlock();
			_cond.notify_one();
			return 0;
		}
		else
		{
			_cond.wait(lock, [&lock]() { return !lock.owns_lock(); });
			return 2;
		}
	}

	std::string_view async_logger::msg_buffer::read()
	{
		if (_buf_begin == _buf_end)
		{
			return std::string_view{};
		}
		size_t len = charToDigit(_buf_begin);
		_useable_bytes.fetch_add(len + sizeof(size_t));
		_using_bytes.fetch_sub(len + sizeof(size_t));
		_buf_begin += sizeof(size_t);
		std::vector<char>::iterator num_it = _buf_begin;
		_buf_begin += len;
		return std::string_view{ num_it, _buf_begin };
	}

	void async_logger::msg_buffer::reset()
	{
		std::lock_guard lock(_ctrl);
		_buf_begin = _buffer.begin();
		_buf_end = _buf_begin;
	}

	std::mutex& async_logger::msg_buffer::getMutex()
	{
		return _ctrl;
	}

	std::condition_variable& async_logger::msg_buffer::getConditionVariable()
	{
		return _cond;
	}

	const std::atomic<size_t>& async_logger::msg_buffer::getUseableBytes() const
	{
		return _useable_bytes;
	}

	const std::atomic<size_t>& async_logger::msg_buffer::getUsingBytes() const
	{
		return _using_bytes;
	}

	size_t async_logger::msg_buffer::charToDigit(std::vector<char>::iterator& it)
	{
		return *reinterpret_cast<size_t*>(_buffer.data() + (it - _buffer.begin()));
	}

	void async_logger::msg_buffer::digitToChar(std::vector<char>::iterator& it, size_t num)
	{
		const char* ptr = reinterpret_cast<const char*>(&num);
		memcpy(_buffer.data() + (it - _buffer.begin()), ptr, sizeof(size_t));
	}

	async_logger::async_logger(const std::string& name, log_level::value min_lv,
		const std::chrono::milliseconds& time)
		: logger_base(name, min_lv)
		, _producer_buffer_arr()
		, _consumer_buffer_arr()
		, _startup_duration(time)
		, _producer_cond()
		, _consumer_cond()
		, _producer_ctrl()
		, _consumer_ctrl()
		, _worker()
	{}

	async_logger::async_logger(const std::string& name, log_level::value min_lv,
		std::vector<std::string>&& format_str_arr, std::vector<std::vector<output_t>>&& output_arr,
		const std::chrono::milliseconds& time)
		: logger_base(name, min_lv, std::move(format_str_arr), std::move(output_arr))
		, _producer_buffer_arr(_format_str_arr.size())
		, _consumer_buffer_arr(_format_str_arr.size())
		, _startup_duration(time)
		, _producer_cond()
		, _consumer_cond()
		, _producer_ctrl()
		, _consumer_ctrl()
		, _worker()
	{
		for (size_t i = 0; i < _format_str_arr.size(); ++i)
		{
			addFmtStrAndOutput_impl();
		}
	}

	async_logger::~async_logger()
	{
		_status.store(2);
		_consumer_cond.notify_all();
		_worker.join();
	}

	void async_logger::setStartupDuration(const std::chrono::milliseconds& time)
	{
		if (_status != 0)
		{
			std::cerr << "logger has started, cannot change duration" << std::endl;
			std::terminate();
		}
		_startup_duration = time;
	}

	void async_logger::destroy_impl() const
	{
		delete this;
	}

	void async_logger::addFmtStrAndOutput_impl()
	{
		_producer_buffer_arr.emplace_back(std::unique_ptr<msg_buffer>{ new msg_buffer{} });
		_consumer_buffer_arr.emplace_back(std::unique_ptr<msg_buffer>{ new msg_buffer{} });
	}

	void async_logger::start_impl()
	{
		std::function<void()> func = std::bind(&async_logger::entry, this);
		_worker = std::thread{ std::move(func) };
	}

	void async_logger::logging(const log_message& msg)
	{
		size_t ret = 0;
		for (size_t i = 0; i < _format_str_arr.size();)
		{
			ret = _producer_buffer_arr[i]->write(msg, _format_str_arr[i]);
			if (ret == 0)
			{
				++i;
			}
			else if (ret == 1)
			{
				_consumer_cond.notify_one();
				std::unique_lock lock(_producer_ctrl, std::try_to_lock_t{});
				auto f = [x = _producer_buffer_arr[i]->getUseableBytes().load(), 
				          y = MAX_FORMATTED_STR_LEN]() { return x >= static_cast<size_t>(y); };
				_producer_cond.wait(lock, f);
			}
			else if (ret == 2)
			{
				continue;
			}
		}
	}

	void async_logger::entry()
	{
		size_t cycle_status = 1;
		output_t_visitor visitor;
		std::unique_lock thread_lock(_consumer_ctrl, std::try_to_lock_t{});
		while (cycle_status)
		{
			if (cycle_status != 2)
			{
				cycle_status = 0;
			}
			for (size_t i = 0; i < _consumer_buffer_arr.size(); ++i)
			{
				while (_consumer_buffer_arr[i]->getUsingBytes() != 0)
				{
					if (cycle_status != 2)
					{
						cycle_status = 1;
					}			
					visitor = _consumer_buffer_arr[i]->read();
					for (auto& output : _output_arr[i])
					{
						std::visit(visitor, output);
					}
				}
			}
			if (cycle_status == 2)
			{
				break;
			}
			if (_status.load() == 2)
			{
				cycle_status = 2;
			}
			if (cycle_status == 0)
			{
				_consumer_cond.wait_for(thread_lock, _startup_duration);
				cycle_status = 1;
			}
			for (size_t i = 0; i < _consumer_buffer_arr.size(); ++i)
			{
				_consumer_buffer_arr[i]->reset();
				{
					std::scoped_lock buffer_lock(_producer_buffer_arr[i]->getMutex(), _consumer_buffer_arr[i]->getMutex());
					_consumer_buffer_arr[i].swap(_producer_buffer_arr[i]);
				}
				_producer_buffer_arr[i]->getConditionVariable().notify_all();
				_consumer_buffer_arr[i]->getConditionVariable().notify_all();
				_producer_cond.notify_all();
			}
		}
	}
}