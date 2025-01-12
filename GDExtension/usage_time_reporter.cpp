#include "usage_time_reporter.h"

#ifdef DEBUG_ENABLED

#ifdef DEV_ENABLED
// #define USE_DEV_SERVER
// #define TEST_FILES_CONTENT
#define PRINT_REQUEST_DATA
#endif

#define DST_PRINT_APP_NAME app_name

DST_GODOT_WARNING_DISABLE()
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/marshalls.hpp>
#include <godot_cpp/classes/time.hpp>
DST_GODOT_WARNING_RESTORE()
using namespace godot;

String UsageTimeReporter::generate_uid() {
	// Random int64
	int64_t r30 = UtilityFunctions::randi();
	int64_t s30 = UtilityFunctions::randi();
	int64_t t4 = UtilityFunctions::randi() & 0xf;
	int64_t rnd = (r30 << 34) + (s30 << 4) + t4;

	// Random data
	String basis_str = DST_FMT_STR("{0} {1} {2} {3} {4} {5}",
			rnd,
			(int64_t)Time::get_singleton()->get_unix_time_from_system(),
			(int)get_os(),
			get_os_version(),
			(int)get_arch(),
			OS::get_singleton()->get_locale());

	return basis_str.md5_text();
}

String UsageTimeReporter::read_user_uid() {
	if (FileAccess::file_exists(user_uid_file)) {
#ifndef TEST_FILES_CONTENT
		auto file = FileAccess::open_encrypted(user_uid_file, FileAccess::ModeFlags::READ, FILE_KEY);
#else
		auto file = FileAccess::open(user_uid_file, FileAccess::ModeFlags::READ);
#endif
		if (file.is_valid()) {
			return file->get_as_text();
		}
	}

	if (FileAccess::file_exists(user_info_file_dd3d_old)) {
		String text;
		{
#ifndef TEST_FILES_CONTENT
			auto file = FileAccess::open_encrypted(user_info_file_dd3d_old, FileAccess::ModeFlags::READ, FILE_KEY);
#else
			auto file = FileAccess::open(user_info_file_dd3d_old, FileAccess::ModeFlags::READ);
#endif
			if (file.is_valid()) {
				text = file->get_as_text();
			}
		}

		if (!text.is_empty()) {
			String DICT_UID = "UID";
			Dictionary info = JSON::parse_string(text);
			if (info.has(DICT_UID)) {
				String old_uid = info[DICT_UID];
				DST_DEV_PRINT("Converting old DD3D UID to a new UID file");
				write_user_uid(old_uid);
				return old_uid;
			}
		}
	}

	String uid = generate_uid();
	write_user_uid(uid);
	return uid;
}

bool UsageTimeReporter::write_user_uid(String uid) {
	DST_DEV_PRINT("Writing user uid to {0}", user_uid_file);

	DirAccess::make_dir_recursive_absolute(user_uid_file.get_base_dir());
#ifndef TEST_FILES_CONTENT
	auto file = FileAccess::open_encrypted(user_uid_file, FileAccess::ModeFlags::WRITE, FILE_KEY);
#else
	auto file = FileAccess::open(user_uid_file, FileAccess::ModeFlags::WRITE);
#endif

	if (file.is_valid()) {
		file->store_string(uid);

		if (file->get_error() != Error::OK) {
			DST_DEV_PRINT_STD_ERR("Error saving uid: %d", file->get_error());
			return false;
		}
		return true;
	}
	return false;
}

Dictionary UsageTimeReporter::read_user_info() {
	if (FileAccess::file_exists(user_info_file)) {
#ifndef TEST_FILES_CONTENT
		auto file = FileAccess::open_encrypted(user_info_file, FileAccess::ModeFlags::READ, FILE_KEY);
#else
		auto file = FileAccess::open(user_info_file, FileAccess::ModeFlags::READ);
#endif
		if (file.is_valid()) {
			return JSON::parse_string(file->get_as_text());
		}
	}

	return Dictionary();
}

bool UsageTimeReporter::write_user_info(Dictionary data, String custom_path) {
	String file_path = custom_path.is_empty() ? user_info_file : custom_path;
	DST_DEV_PRINT("Writing user info to {0}", file_path);

	DirAccess::make_dir_recursive_absolute(file_path.get_base_dir());
#ifndef TEST_FILES_CONTENT
	auto file = FileAccess::open_encrypted(file_path, FileAccess::ModeFlags::WRITE, FILE_KEY);
#else
	auto file = FileAccess::open(file_path, FileAccess::ModeFlags::WRITE);
#endif

	if (file.is_valid()) {
		file->store_string(JSON::stringify(data));

		if (file->get_error() != Error::OK) {
			DST_DEV_PRINT_STD_ERR("Error saving info: %d", file->get_error());
			return false;
		}
		return true;
	}
	return false;
}

UsageTimeReporter::OSEnum UsageTimeReporter::get_os() {
	String name = OS::get_singleton()->get_name();

	if (name == "Windows") {
		return OSEnum::Windows;
	} else if (name == "macOS") {
		return OSEnum::macOS;
	} else if (name == "Linux") {
		return OSEnum::Linux;
	} else if (Array::make("FreeBSD", "NetBSD", "OpenBSD", "BSD").has(name)) {
		return OSEnum::Linux;
	} else if (name == "Android") {
		return OSEnum::Android;
	} else if (name == "iOS") {
		return OSEnum::iOS;
	} else if (name == "Web") {
		return OSEnum::Web;
	}

	return OSEnum::Undefined;
}

UsageTimeReporter::OSArch UsageTimeReporter::get_arch() {
	String arch = Engine::get_singleton()->get_architecture_name();

	if (arch == "x86_32") {
		return OSArch::x86_32;
	} else if (arch == "x86_64") {
		return OSArch::x86_64;
	} else if (arch == "arm32") {
		return OSArch::Arm32;
	} else if (arch == "arm64") {
		return OSArch::Arm64;
	} else if (arch == "riscv") {
		return OSArch::RiscV32;
	} else if (arch == "rv64") {
		return OSArch::RiscV64;
	} else if (arch == "wasm32") {
		return OSArch::Wasm32;
	} else if (arch == "wasm64") {
		return OSArch::Wasm64;
	} else if (arch == "e2k") {
		return OSArch::E2K;
	}

	return OSArch::Undefined;
}

String UsageTimeReporter::get_os_version() {
	switch (get_os()) {
		case OSEnum::Linux: {
			return DST_FMT_STR("{0} {1}", OS::get_singleton()->get_distribution_name(), OS::get_singleton()->get_version());
		}
		case OSEnum::Windows:
		case OSEnum::Android:
		case OSEnum::iOS:
		case OSEnum::macOS: {
			return OS::get_singleton()->get_version();
		}
		case OSEnum::Web: {
			return "Web";
		}
	}
	return "Undefined";
}

String UsageTimeReporter::get_editor_language() {
	// TranslationServer is obtained dynamically to avoid compiling it as part of a library.
	Object *ts = Engine::get_singleton()->get_singleton("TranslationServer");
	if (ts) {
		Variant lang_var = ts->call("get_tool_locale");
		return lang_var.booleanize() ? String(lang_var) : "en";
	}
	return "en";
}

UsageTimeReporter::LangRegion UsageTimeReporter::get_system_language() {
	// This string can have a different number of parts...
	// language_Script_COUNTRY_VARIANT@extra
	PackedStringArray os_locale_split = OS::get_singleton()->get_locale().split("_");

	String sys_lang = os_locale_split[0];
	String sys_region = "Unknown";

	if (os_locale_split.size() == 2) {
		sys_region = os_locale_split[1];
	} else {
		if (os_locale_split.size() >= 3 && os_locale_split[1].length() == 4) {
			sys_region = os_locale_split[2];
		} else if (os_locale_split.size() >= 2) {
			sys_region = os_locale_split[1];
		}
	}

	return LangRegion(sys_lang.substr(0, 3), sys_region.substr(0, 3));
}

void UsageTimeReporter::request_completed(RequestResult result) {
#ifdef DEV_ENABLED
	auto dict_to_array = [](Dictionary dict) {
		PackedStringArray res;
		auto keys = dict.keys();
		for (int i = 0; i < dict.size(); i++) {
			res.append(DST_FMT_STR("{0}: {1}", keys[i], dict[keys[i]]));
		}
		return res;
	};
	auto body_to_array = [&dict_to_array](String body) -> PackedStringArray {
		if (body.is_empty()) {
			return PackedStringArray(Array::make(body));
		}

		Ref<JSON> json;
		json.instantiate();
		Error err = json->parse(body);
		if (err != Error::OK) {
			return PackedStringArray(Array::make(body));
		}

		Variant p = json->get_data();
		if (p.get_type() == Variant::Type::STRING) {
			return PackedStringArray(Array::make(body));
		}

		if (p.get_type() == Variant::DICTIONARY) {
			return dict_to_array(p);
		} else if (p.get_type() == Variant::ARRAY) {
			return PackedStringArray(p);
		}
		return PackedStringArray(Array::make(body));
	};

	PackedStringArray headers = dict_to_array(result.headers);
	PackedStringArray body = body_to_array(result.body);
	PackedStringArray request_body = body_to_array(result.request_body);
	String indent = String("\n\t\t");

#ifdef PRINT_REQUEST_DATA
	DST_DEV_PRINT_STD("Request completed.\n\tCode: %d\n\tBody:\n\t\t%s\n\tHeaders:\n\t\t%s\n\tRequest URL: %s\n\tRequest Headers:\n\t\t%s\n\tRequest Body:\n\t\t%s",
			result.code, indent.join(body).utf8(), indent.join(headers).utf8(), result.request_url.utf8(), indent.join(result.request_headers).utf8(), indent.join(request_body).utf8());
#else
	DST_DEV_PRINT_STD("Request completed.\n\tCode: %d", result.code);
#endif
#endif
}

void UsageTimeReporter::submit_data() {
	if (current_session_uid != 0) {
		DST_DEV_PRINT_STD_ERR(DST_NAMEOF(UsageTimeReporter) ": A startup request has already been made with the Session UID: %d\n", current_session_uid);
		return;
	}

	std::vector<RequestData> queue;

	Dictionary info = read_user_info();
	String uuid = read_user_uid();

	if (info.has(DICT_STOP_GROUP)) {
		TypedArray<Dictionary> data = info[DICT_STOP_GROUP];

		for (int i = 0; i < data.size(); i++) {
			Dictionary d = data[i];

			auto body = SubmitStopDurationTime(
					uuid,
					app_id,
					d[DICT_STOP_ID],
					d[DICT_STOP_DURATION]);

			queue.push_back(RequestData(stop_address, JSON::stringify(body.serialize()), default_request_headers));
		}

		info.erase(DICT_STOP_GROUP);
		DST_DEV_PRINT("Updating info file before sending");
		write_user_info(info);
	}

	if (is_need_to_send_new_data) {
		auto sys_lang = get_system_language();
		current_session_uid = (int64_t)Time::get_singleton()->get_unix_time_from_system();
		Dictionary godot_version = Engine::get_singleton()->get_version_info();

		auto body = ClientSubmitDataDeviceAndProduct(
				uuid,
				app_id,
				current_session_uid,

				get_editor_language(),
				sys_lang.lang,
				sys_lang.region,
				get_os(),
				get_os_version().substr(0, 64),
				get_arch(),

				DST_FMT_STR("{0}:{1}.{2}.{3}", library_version, godot_version["major"], godot_version["minor"], godot_version["patch"]).substr(0, 32),
				is_telemetry_refused);

		queue.push_back(RequestData(start_address, JSON::stringify(body.serialize()), default_request_headers));
	}

	send_data(queue);
}

void UsageTimeReporter::send_data(std::vector<RequestData> &queue) {
	if (queue.size()) {
		http_thread = std::jthread([&](std::stop_token token, std::vector<RequestData> queue) {
			for (const auto &data : queue) {
				_connection_body(data, token);
			}
		},
				queue);
	}
}

void UsageTimeReporter::_connection_body(const RequestData data, std::stop_token &token) {
	Ref<HTTPClient> http = nullptr;
	http.instantiate();

	Error err = Error::OK;

	if (http.is_valid() && !token.stop_requested()) {
#ifdef USE_DEV_SERVER
		http->connect_to_host(telemetry_domain, 7443);
#else
		http->connect_to_host(telemetry_domain);
#endif

		err = http->poll();
		if (err != Error::OK) {
			DST_DEV_PRINT_STD_ERR("Failed to initialize connection. Error: %s", UtilityFunctions::error_string(err).utf8());
			return;
		}
	} else {
		return;
	}

	HTTPClient::Status prev_status = HTTPClient::STATUS_DISCONNECTED;
	while (http.is_valid() && !token.stop_requested()) {
		err = http->poll();
		if (err != Error::OK) {
			DST_DEV_PRINT_STD_ERR("Failed to connect. Error: %s", UtilityFunctions::error_string(err).utf8());
			return;
		}

		HTTPClient::Status status = http->get_status();
		switch (status) {
			case godot::HTTPClient::STATUS_DISCONNECTED:
			case godot::HTTPClient::STATUS_CONNECTION_ERROR:
			case godot::HTTPClient::STATUS_CANT_RESOLVE:
			case godot::HTTPClient::STATUS_CANT_CONNECT:
			case godot::HTTPClient::STATUS_TLS_HANDSHAKE_ERROR:
				DST_DEV_PRINT_STD_ERR("Connection error: %d", status);
				return;
			case godot::HTTPClient::STATUS_RESOLVING:
			case godot::HTTPClient::STATUS_CONNECTING:
			case godot::HTTPClient::STATUS_REQUESTING:
			case godot::HTTPClient::STATUS_BODY:
			default:
				if (status != prev_status) {
					DST_DEV_PRINT_STD("Connecting status: %d", status);
				}
				break;
			case godot::HTTPClient::STATUS_CONNECTED: {
				// REQUEST
				RequestResult res = _request_body(http, data, token);
				request_completed(res);
				return;
			}
		}
		prev_status = status;
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

UsageTimeReporter::RequestResult UsageTimeReporter::_request_body(Ref<HTTPClient> http, const RequestData &data, std::stop_token &token) {
	Error err = http->request(HTTPClient::METHOD_POST, data.url, data.headers, data.body);
	if (err != Error::OK) {
		DST_DEV_PRINT_STD_ERR("Failed to create a request. Error: %s", UtilityFunctions::error_string(err).utf8());
		return RequestResult(HTTPClient::ResponseCode::RESPONSE_BAD_REQUEST, "", Dictionary(), data.url, data.headers, data.body);
	}

	while (http.is_valid() && !token.stop_requested()) {
		err = http->poll();
		if (err != Error::OK) {
			DST_DEV_PRINT_STD_ERR("Failed to poll() \"%s\". Error: %s", (telemetry_domain + start_address).utf8(), UtilityFunctions::error_string(err).utf8());
			return RequestResult(HTTPClient::ResponseCode::RESPONSE_BAD_REQUEST, "", Dictionary(), data.url, data.headers, data.body);
		}

		auto get_body = [&http]() {
			PackedByteArray res;
			PackedByteArray tmp = http->read_response_body_chunk();
			while (!tmp.is_empty()) {
				res.append_array(tmp);
				if (http->get_status() != HTTPClient::STATUS_BODY) {
					break;
				}
				tmp = http->read_response_body_chunk();
			}
			return res.get_string_from_utf8();
		};

		HTTPClient::ResponseCode code = (HTTPClient::ResponseCode)http->get_response_code();
		if (code == HTTPClient::ResponseCode::RESPONSE_OK) {
			// RESULT
			return RequestResult(code, get_body(), http->get_response_headers_as_dictionary(), data.url, data.headers, data.body);
		} else {
			if (code != 0) {
				DST_DEV_PRINT_STD_ERR("Failed to get a response from \"%s\". Code: %d", (telemetry_domain + start_address).utf8(), code);
				return RequestResult(code, get_body(), http->get_response_headers_as_dictionary(), data.url, data.headers, data.body);
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	return RequestResult(HTTPClient::ResponseCode::RESPONSE_NO_CONTENT, "", Dictionary(), data.url, data.headers, data.body);
}

UsageTimeReporter::UsageTimeReporter(String app_name, String app_id, String library_version, String root_settings, String host_url, String config_file_name) {
	library_version = library_version;

	String base_config_path = OS::get_singleton()->get_config_dir().path_join("DmitriySalnikov");
	user_uid_file = base_config_path.path_join("telemetry_uid.txt");

	user_info_file_dd3d_old = base_config_path.path_join("telemetry_info.json");

	this->app_id = app_id;
	this->app_name = app_name;
	this->user_info_file = base_config_path.path_join(config_file_name);

	FILE_KEY = Marshalls::get_singleton()->base64_to_raw(TELEMETRY_DST_FILE_KEY);

#ifdef USE_DEV_SERVER
	DST_PRINT("!!! A local telemetry server is used. !!!");
	telemetry_domain = "https://localhost";
#else
	// https://example.com
	telemetry_domain = host_url;
#endif

	start_address = "/submit/start";
	stop_address = "/submit/stop_duration";

	accept_type_header = "accept: */*";
	api_version_header = "telemetry-api-version: 1.0";
	body_type_header = "Content-Type: application/json";

	default_request_headers = PackedStringArray(Array::make(accept_type_header, api_version_header, body_type_header));

	s_root_addon_setting = root_settings;
	s_telemetry_setting = "telemetry_state";

#ifdef DEV_ENABLED
	if (Engine::get_singleton()->is_editor_hint()) {
		Dictionary version = Engine::get_singleton()->get_version_info();
		bool has_editor_interface_singleton = (((int)version["major"]) >= 4 && ((int)version["minor"]) >= 2);
		Object *ts = Engine::get_singleton()->get_singleton("TranslationServer");

		String basis_str = DST_FMT_STR("\tOS id: {0}\n\tOS version: {1}\n\tOS name: {2}\n\tEngine version: {3}\n\tArch id: {4}\n\tArch name: {5}\n\tLocale: {6}\n\tUser info: {7}\n\tEditorInterface object: {8}\n\tTranslationServer object: {9}\n\tEditor language: {10}\n",
				(int)get_os(),
				get_os_version(),
				OS::get_singleton()->get_name(),
				version,
				(int)get_arch(),
				Engine::get_singleton()->get_architecture_name(),
				OS::get_singleton()->get_locale(),
				user_info_file,
				has_editor_interface_singleton ? Engine::get_singleton()->get_singleton("EditorInterface") : nullptr,
				Engine::get_singleton()->get_singleton("TranslationServer"),
				ts ? ts->call("get_tool_locale") : "TranslationServer not found");
		DST_DEV_PRINT_STD("System and Engine info: \n%s", basis_str.utf8());
	}
#endif

	_setup_settings();

	// must be zero before the start
	current_session_uid = 0;

	if (Engine::get_singleton()->is_editor_hint()) {
		submit_data();
	}
}

UsageTimeReporter::~UsageTimeReporter() {
	if (is_need_to_send_new_data) {
		save_data_on_closing();
	}
}

#define DST_EDITOR_DEFINE_SETTING_AND_GET_HINT(_editor_settings, var, path, def, type, hint, hint_string) \
	{                                                                                                     \
		if (!_editor_settings->call(DST_NAMEOF(has_setting), path)) {                                     \
			_editor_settings->call(DST_NAMEOF(set_setting), path, def);                                   \
		}                                                                                                 \
		Dictionary info;                                                                                  \
		info["name"] = path;                                                                              \
		info["type"] = type;                                                                              \
		info["hint"] = hint;                                                                              \
		info["hint_string"] = hint_string;                                                                \
		_editor_settings->call(DST_NAMEOF(add_property_info), info);                                      \
		_editor_settings->call(DST_NAMEOF(set_initial_value), path, def, false);                          \
	}                                                                                                     \
	var = _editor_settings->call(DST_NAMEOF(get_setting), path)

void UsageTimeReporter::_setup_settings() {
	Dictionary version = Engine::get_singleton()->get_version_info();
	bool has_editor_interface_singleton = (((int)version["major"]) >= 4 && ((int)version["minor"]) >= 2);
	Ref<Resource> editor_settings;

	if (has_editor_interface_singleton) {
		Object *ei = Engine::get_singleton()->get_singleton("EditorInterface");
		if (ei) {
			editor_settings = ei->call("get_editor_settings");
		}
	}

	has_editor_interface_singleton = has_editor_interface_singleton && editor_settings.is_valid();
	int initial_debug_state = 0;

	if (has_editor_interface_singleton) {
		DST_EDITOR_DEFINE_SETTING_AND_GET_HINT(editor_settings, initial_debug_state, String(s_root_addon_setting) + s_telemetry_setting, 0, Variant::INT, PROPERTY_HINT_ENUM, "First launch,Share anonymous data,Refuse to share data");
	} else {
		DST_DEFINE_SETTING_AND_GET_HINT(initial_debug_state, String(s_root_addon_setting) + s_telemetry_setting, 0, Variant::INT, PROPERTY_HINT_ENUM, "First launch,Share anonymous data,Refuse to share data");
	}

	bool is_changed = false;
	Dictionary info = read_user_info();
	if (initial_debug_state == 0) {
		if (has_editor_interface_singleton) {
			editor_settings->call(DST_NAMEOF(set_setting), String(s_root_addon_setting) + s_telemetry_setting, 1);
		} else {
			DST_PS()->set_setting(String(s_root_addon_setting) + s_telemetry_setting, 1);
			DST_PS()->save();
		}

		String refuse_str = DST_FMT_STR("But you can opt out of this in the {0} settings '{1}'.", has_editor_interface_singleton ? "editor" : "project", String(s_root_addon_setting) + s_telemetry_setting);

		DST_PRINT_WARNING(app_name + " sends anonymous data about the library usage time. This is done so that the developer knows that the time spent on library development is not wasted. " + refuse_str);
		is_telemetry_refused = false;

		is_need_to_send_new_data = true;
		if (info.has(DICT_REFUSED)) {
			info.erase(DICT_REFUSED);
			DST_DEV_PRINT("Mark info as changed: Erase DICT_REFUSED on Init state");
			is_changed = true;
		}
	} else if (initial_debug_state == 1) {
		is_telemetry_refused = false;

		is_need_to_send_new_data = true;
		if (info.has(DICT_REFUSED)) {
			info.erase(DICT_REFUSED);
			DST_DEV_PRINT("Mark info as changed: Erase DICT_REFUSED on Share state");
			is_changed = true;
		}
	} else if (initial_debug_state == 2) {
		is_telemetry_refused = true;

		if (info.has(DICT_REFUSED)) {
			if (!info[DICT_REFUSED]) {
				info[DICT_REFUSED] = true;
				DST_DEV_PRINT("Mark info as changed: Set DICT_REFUSED true on Refuse state");
				is_changed = true;
			}
			is_need_to_send_new_data = false;
		} else {
			info[DICT_REFUSED] = false;
			is_need_to_send_new_data = true;
			DST_DEV_PRINT("Mark info as changed: Set DICT_REFUSED false on Refuse state");
			is_changed = true;
		}
	}

	if (is_changed) {
		write_user_info(info);
	}
}

#undef EDITOR_DEFINE_SETTING_AND_GET_HINT

void UsageTimeReporter::save_data_on_closing() {
	Dictionary info = read_user_info();
	DST_DEV_PRINT("Saving user info to {0}", user_info_file);

	if (!info.has(DICT_REFUSED) || !info[DICT_REFUSED]) {
		TypedArray<Dictionary> data = info.get(DICT_STOP_GROUP, TypedArray<Dictionary>());

		Dictionary end = DSTGodotUtils::make_dict(
				DICT_STOP_DURATION, (int)Math::round(Time::get_singleton()->get_ticks_msec() / 1000.f),
				DICT_STOP_ID, current_session_uid);

		data.push_back(end);
		info[DICT_STOP_GROUP] = data;

		DST_DEV_PRINT("User info:\n{0}", info);
		write_user_info(info);
	}
}

void UsageTimeReporterGodotObj::_bind_methods() {
	ClassDB::bind_method(D_METHOD(DST_NAMEOF(_deferred_call)), &UsageTimeReporterGodotObj::_deferred_call);
}

void UsageTimeReporterGodotObj::_deferred_call() {
	// disable by default for headless
	if (Engine::get_singleton()->is_editor_hint()) {
		if (Engine::get_singleton()->has_singleton("DisplayServer")) {
			if (Object *display_server = Engine::get_singleton()->get_singleton("DisplayServer"); display_server) {
				if (display_server->call("window_can_draw", 0)) {
					usage_reporter = std::make_unique<UsageTimeReporter>(app_name, app_id, library_version, root_settings, host_url, config_file_name);
				}
			}
		}
	}
}

UsageTimeReporterGodotObj::UsageTimeReporterGodotObj(String app_name, String app_id, String library_version, String root_settings, String host_url, String config_file_name) :
		app_name(app_name),
		app_id(app_id),
		library_version(library_version),
		root_settings(root_settings),
		host_url(host_url),
		config_file_name(config_file_name) {
	call_deferred(DST_NAMEOF(_deferred_call));
}

#endif
