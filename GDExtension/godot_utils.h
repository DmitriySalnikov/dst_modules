#pragma once

// Dmitriy Salnikov Telemetry - DST
#define DST_NAMEOF(s) #s

#if _MSC_VER

#define DST_GODOT_WARNING_DISABLE() \
	__pragma(warning(disable : 4244 26451 26495))
#define DST_GODOT_WARNING_RESTORE() \
	__pragma(warning(default : 4244 26451 26495))

#define DST_MSVC_WARNING_DISABLE(n) __pragma(warning(disable \
													 : n))
#define DST_MSVC_WARNING_RESTORE(n) __pragma(warning(default \
													 : n))
#else

#define DST_GODOT_WARNING_DISABLE(n)
#define DST_GODOT_WARNING_RESTORE(n)

#define DST_MSVC_WARNING_DISABLE(n)
#define DST_MSVC_WARNING_RESTORE(n)
#endif

DST_GODOT_WARNING_DISABLE()
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/variant.hpp>
DST_GODOT_WARNING_RESTORE()

#pragma region PRINTING

// Define this before calling any printing functions
// #define DST_PRINT_APP_NAME YOUR_APP_NAME_VARIABLE

#define DST_FMT_STR(str, ...) String(str).format(Array::make(__VA_ARGS__))
#define DST_PRINT(text, ...) DSTGodotUtils::_logv(DST_PRINT_APP_NAME, false, DST_FMT_STR(godot::Variant(text), ##__VA_ARGS__).utf8().get_data())
#define DST_PRINT_ERROR(text, ...) godot::_err_print_error(__FUNCTION__, DSTGodotUtils::get_file_name_in_repository(__FILE__).utf8().get_data(), __LINE__, DST_FMT_STR(godot::Variant(text).stringify(), ##__VA_ARGS__))
#define DST_PRINT_WARNING(text, ...) godot::_err_print_error(__FUNCTION__, DSTGodotUtils::get_file_name_in_repository(__FILE__).utf8().get_data(), __LINE__, DST_FMT_STR(godot::Variant(text).stringify(), ##__VA_ARGS__), false, true)

#if DEV_ENABLED
#define DST_DEV_PRINT(text, ...) DST_PRINT(text, ##__VA_ARGS__)
#define DST_DEV_PRINT_STD(format, ...) DSTGodotUtils::_logv(DST_PRINT_APP_NAME, false, format, ##__VA_ARGS__)
#define DST_DEV_PRINT_STD_ERR(format, ...) DSTGodotUtils::_logv(DST_PRINT_APP_NAME, true, format, ##__VA_ARGS__)
#else
#define DST_DEV_PRINT(text, ...)
#define DST_DEV_PRINT_STD(format, ...)
#define DST_DEV_PRINT_STD_ERR(format, ...)
#endif

#pragma endregion !PRINTING

#define DST_PS() ProjectSettings::get_singleton()
#define DST_DEFINE_SETTING(path, def, type)     \
	{                                           \
		if (!DST_PS()->has_setting(path)) {     \
			DST_PS()->set_setting(path, def);   \
		}                                       \
		Dictionary info;                        \
		info["name"] = path;                    \
		info["type"] = type;                    \
		DST_PS()->add_property_info(info);      \
		DST_PS()->set_initial_value(path, def); \
	}
#define DST_DEFINE_SETTING_AND_GET(var, path, def, type) \
	{                                                    \
		if (!DST_PS()->has_setting(path)) {              \
			DST_PS()->set_setting(path, def);            \
		}                                                \
		Dictionary info;                                 \
		info["name"] = path;                             \
		info["type"] = type;                             \
		DST_PS()->add_property_info(info);               \
		DST_PS()->set_initial_value(path, def);          \
	}                                                    \
	var = DST_PS()->get_setting(path)
#define DST_DEFINE_SETTING_AND_GET_HINT(var, path, def, type, hint, hint_string) \
	{                                                                            \
		if (!DST_PS()->has_setting(path)) {                                      \
			DST_PS()->set_setting(path, def);                                    \
		}                                                                        \
		Dictionary info;                                                         \
		info["name"] = path;                                                     \
		info["type"] = type;                                                     \
		info["hint"] = hint;                                                     \
		info["hint_string"] = hint_string;                                       \
		DST_PS()->add_property_info(info);                                       \
		DST_PS()->set_initial_value(path, def);                                  \
	}                                                                            \
	var = DST_PS()->get_setting(path)
#define DST_DEFINE_SETTING_READ_ONLY(path, def, type) \
	{                                                 \
		DST_PS()->set_setting(path, def);             \
		Dictionary info;                              \
		info["name"] = path;                          \
		info["type"] = type;                          \
		/* Does not work in the ProjectSettings */    \
		info["usage"] = PROPERTY_USAGE_READ_ONLY;     \
		DST_PS()->add_property_info(info);            \
		DST_PS()->set_initial_value(path, def);       \
	}

#include <stdarg.h>

namespace DSTGodotUtils {
static godot::String get_file_name_in_repository(const godot::String &name) {
	if (name != "") {
		int64_t idx = name.find("src");
		if (idx != -1)
			return name.substr(idx, name.length());
	}
	return name;
}

static void _logv(const godot::String &app_name, bool p_err, const char *p_format, ...) {
#if DEBUG_ENABLED
	const int static_buf_size = 512;
	char static_buf[static_buf_size];
	char *buf = static_buf;

	va_list list_copy;
	va_start(list_copy, p_format);

	va_list p_list;
	va_copy(p_list, list_copy);
	int len = vsnprintf(buf, static_buf_size, p_format, p_list);
	va_end(p_list);

	std::string s;

	DST_MSVC_WARNING_DISABLE(6387)

	if (len >= static_buf_size) {
		char *buf_alloc = (char *)malloc((size_t)len + 1);
		vsnprintf(buf_alloc, (size_t)len + 1, p_format, list_copy);
		s = buf_alloc;
		free(buf_alloc);
	} else {
		s = buf;
	}
	va_end(list_copy);

	DST_MSVC_WARNING_RESTORE(6387)

	if (p_err) {
		fprintf(stderr, "[Error] DST Telem (%s): %s\n", app_name.utf8().get_data(), s.c_str());
	} else {
		printf("[Info] DST Telem (%s): %s\n", app_name.utf8().get_data(), s.c_str());
		// fflush(stdout);
	}
#endif
}

template <typename T, typename Key, typename Value>
static T dict_append_all(T appendable, Key key, Value val) {
	appendable[key] = val;
	return appendable;
}

template <typename T, typename Key, typename Value, typename... Args>
static T dict_append_all(T appendable, Key key, Value val, Args... args) {
	appendable[key] = val;
	return dict_append_all(appendable, args...);
}

template <typename T>
static T dict_append_all(T appendable) {
	return appendable;
}

template <class... Args>
static godot::Dictionary make_dict(Args... args) {
	return dict_append_all(godot::Dictionary(), args...);
}
} // namespace DSTGodotUtils
