#pragma once
#include <type_traits>
#include <filesystem>
#include <syncstream>
#include <iostream>
#include <fstream>
#include <format>
#include <atomic>
#include <mutex>

#include "compatible.hpp"
#include "level.hpp"

namespace my_log
{
	template<typename Class_type>
	class output_base
	{
	protected:
		output_base() = default;
		output_base(const output_base&) = delete;
		output_base(output_base&&) = default;
	public:
		// need override
		void landing(std::string_view& input)
		{
			static_cast<Class_type*>(this)->landing(input);
		}

		// need override
		void destory() const
		{
			static_cast<Class_type*>(this)->destory();
		}
	};

	class screen_output : public output_base<screen_output>
	{
		friend class output_base;
	public:
		screen_output() = default;

		void destroy() const
		{
			delete this;
		}
	private:
		void landing(std::string_view& input)
		{
			std::cout << input << std::endl;
		}
	};

	class file_output : public output_base<file_output>
	{
		friend class output_base;
	public:
		file_output(const std::filesystem::path& name)
			: _fout(name, std::ios::out)
			, _synout(_fout)
		{}

		~file_output()
		{
			_fout.flush();
			_fout.close();
		}

		file_output(const file_output& other) = delete;
		file_output(file_output&& other) noexcept
			: _fout(std::move(other._fout))
			, _synout(std::move(other._synout))
		{}

		void destory() const
		{
			delete this;
		}
	private:
		void landing(std::string_view& input)
		{
			_synout << input << std::endl;
		}
	private:
		std::ofstream _fout;
		std::osyncstream _synout;
	};

	class roll_file_output : public output_base<roll_file_output>
	{
		friend class output_base;
	public:
		roll_file_output(const std::filesystem::path& name_base, size_t single_file_size = 0)
			: _name_base(name_base)
			, _fout(nameGenerator(), std::ios::out)
			, _synout(_fout)
			, _written_byte(0)
			, _max_single_file_size(defaultSingleFileSize())
		{
			setMaxSingleFileSize(single_file_size);
		}

		~roll_file_output()
		{
			_fout.flush();
			_fout.close();
		}

		roll_file_output(const roll_file_output& other) = delete;
		roll_file_output(roll_file_output&& other) noexcept
			: _name_base(std::move(other._name_base))
			, _fout(std::move(other._fout))
			, _synout(std::move(other._synout))
			, _written_byte(other._written_byte.load())
			, _max_single_file_size(other._max_single_file_size.load())
			, _ctrl()
		{}

		void destory() const
		{
			delete this;
		}

		const std::atomic<size_t>& getMaxSingleFileSize() noexcept
		{
			return _max_single_file_size;
		}

		void setMaxSingleFileSize(size_t num) noexcept
		{
			if (num < defaultMinSingleFileSize())
			{
				_max_single_file_size.store(defaultMinSingleFileSize());
			}
			_max_single_file_size.store(num);
		}

		std::filesystem::path nameGenerator() const noexcept
		{
			std::filesystem::path name = _name_base;
			name.append(std::format(" {0:%T}", std::chrono::system_clock::now()));
			return name;
		}

		constexpr static size_t defaultSingleFileSize() noexcept
		{
			return static_cast<size_t>(1024) * 1024 * 10;
		}

		constexpr static size_t defaultMinSingleFileSize() noexcept
		{
			return static_cast<size_t>(1024) * 16;
		}
	private:
		void landing(std::string_view& input)
		{
			if (_written_byte.load() + input.size() > _max_single_file_size.load())
			{
				std::lock_guard lock(_ctrl);
				if (_written_byte.load() + input.size() > _max_single_file_size.load())
				{
					_fout.flush();
					_fout.close();
					_fout.open(nameGenerator(), std::ios::out);
					_written_byte.store(0);
				}
			}
			_synout << input << std::endl;
			_written_byte.fetch_add(input.size());
		}
	private:
		std::filesystem::path _name_base;
		std::ofstream _fout;
		std::osyncstream _synout;
		std::atomic<size_t> _written_byte;
		std::atomic<size_t> _max_single_file_size;
		std::mutex _ctrl;
	};
}