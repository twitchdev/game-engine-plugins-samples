#pragma once
/** \file */
#include <string>
#include <utility>
#include <variant>
#include <type_traits>
#include <cstdio>
#include <memory>
#include <stdexcept>

namespace TwitchSDK {
	/**
	 * @brief The char type the TwitchSDK library was compiled with.
	 *
	 * This is defined as:
	 * - \c wchar_t if \c TWITCHSDK_STR_WSTRING is defined
	 * - \c char16_t if \c TWITCHSDK_STR_U16STRING is defined
	 * - \c char otherwise
	 */
#if defined(TWITCHSDK_STR_WSTRING)
	using twitchsdkchar = wchar_t;
	#define STR(x) L##x
#elif defined(TWITCHSDK_STR_U16STRING)
	using twitchsdkchar = char16_t;
	#define STR(x) u##x
#else
	using twitchsdkchar = char;
	#define STR(x) x
#endif

	/**
	 * @brief The string type used by the TwitchSDK library.
	 *
	 * @see @ref twitchsdkchar
	 */
	using string = std::basic_string<twitchsdkchar>;
	/**
	 * @brief The string_view type used by the TwitchSDK library.
	 *
	 * @see @ref twitchsdkchar
	 */
	using string_view = std::basic_string_view<twitchsdkchar>;

	/**
	 * @class StringHolder twitchsdk_strings.hpp twitchsdk.hpp
	 * @brief Holds either a by-value or a by-reference string.
	 *
	 * This class contains either a string or a string_view.
	 * It enables you to use a string_view whenever possible.
	 */
	template<class TChar> class StringHolder {
		using string = std::basic_string<TChar>;
		using string_view = std::basic_string_view<TChar>;
		std::variant<string, string_view> String;

	public:
		StringHolder() : String(string_view()) {}
		StringHolder(string_view s) : String(s) {}
		StringHolder(string&& s) : String(s) {}

		// When moving, we move along as we are.
		// However, when copying, we always produce an owned StringHolder.
		//
		// The point of a StringHolder is that you actually want to return a string_view,
		// exept in cases where that is not possible. The StringHolder itself is mainly
		// intended to be passed to APIs that take a string_view.
		//
		// So this implementation assumes that performance-critical code paths where you
		// really want to avoid copying and can also ensure that the backing memory
		// stays alive are always going to use std::move().
		// If you don't move and instead just copy, our copy constructor makes sure that
		// you get an owned instance that can be stored safely.
		StringHolder(const StringHolder& s) : String(s.ToOwned()) {}
		StringHolder(StringHolder&& s) noexcept : String(std::move(s.String)) {}
		StringHolder& operator=(const StringHolder& s) {
			String = s.ToOwned();
			return *this;
		}
		StringHolder& operator=(StringHolder&& s) noexcept {
			String = std::move(s.String);
			return *this;
		}

		/** @brief String data, not null-terminated (like string_view.data()). */
		const TChar* data() const
		{
			if (std::holds_alternative<string>(String))
			{
				return std::get<string>(String).c_str();
			}
			else
			{
				return std::get<string_view>(String).data();
			}
		}

		/** @brief String length */
		const size_t size() const
		{
			if (std::holds_alternative<string>(String))
			{
				return std::get<string>(String).size();
			}
			else
			{
				return std::get<string_view>(String).size();
			}
		}

		/** @brief Convert to owned string */
		string ToOwned() const
		{
			if (std::holds_alternative<string>(String))
			{
				return std::get<string>(String);
			}
			else
			{
				return string(std::get<string_view>(String));
			}
		}

		operator string_view() const
		{
			if (std::holds_alternative<string>(String))
			{
				return std::get<string>(String);
			}
			else
			{
				return std::get<string_view>(String);
			}
		}

		friend string operator+(string lhs, const StringHolder& rhs) {
			return lhs + rhs.ToOwned();
		}

		bool operator!=(const StringHolder& other) const {
			return !(*this == other);
		}
		bool operator==(const StringHolder& other) const {
			return *this == ((string_view)other);
		}

		friend bool operator==(const StringHolder& lhs, string_view rhs) {
			return ((string_view)lhs) == rhs;
		}
	};
	/**
	 * @brief The @ref StringHolder type used by the TwitchSDK library.
	 *
	 * @see @ref twitchsdkchar
	 */
	using string_holder = StringHolder<twitchsdkchar>;
#ifndef DOXYGEN_SHOULD_SKIP_THIS
	using u16string_holder = StringHolder<char16_t>;
	using stdstring_holder = StringHolder<std::string::value_type>;

	template <class T, class... Ts> struct is_any : std::disjunction<std::is_same<T, Ts>...> {};
	template<class> inline constexpr bool always_false_v = false;
	template<class T>
	auto format_string_internal_convert(T&& t) {
		using D = std::decay_t<std::remove_cv_t<std::remove_reference_t<T>>>;
		if constexpr (std::is_same_v<D, std::string>) {
			return t.c_str();
		} else if constexpr (std::is_same_v<D, std::string_view>) {
			return t.data();
		} else if constexpr (std::is_same_v<D, StringHolder<std::string::value_type>>) {
			return t.data();
		} else if constexpr (is_any<D, const char*, int>::value) {
			return t;
		} else {
			static_assert(always_false_v<T>, "Unsupported format string item");
			return std::forward<T>(t);
		}
	}

	template<typename ... Args>
	std::string format_string_internal(std::string_view format, Args&& ... args)
	{
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
#endif
		int isize = std::snprintf(nullptr, 0, format.data(), std::forward<Args>(args) ...);
		if (isize <= 0) { throw std::runtime_error("Error during formatting."); }
		auto size = static_cast<size_t>(isize) + 1;
		auto buf = std::make_unique<char[]>(size);
		std::snprintf(buf.get(), size, format.data(), args ...);
		return std::string(buf.get(), buf.get() + size - 1);
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
	}

	template<typename ... Args>
	std::string format_string(std::string_view fmt, Args&& ... args) {
		return format_string_internal(fmt, format_string_internal_convert(std::forward<Args>(args))...);
	}

	string_holder from_utf8(std::string_view s);
	stdstring_holder to_utf8(string_view s);
#endif
}
