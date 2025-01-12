#pragma once

#ifdef DEBUG_ENABLED

#include "godot_utils.h"

#include "jthread.hpp"
#include <array>

DST_GODOT_WARNING_DISABLE()
#include <godot_cpp/classes/http_client.hpp>
DST_GODOT_WARNING_RESTORE()
using namespace godot;

class UsageTimeReporter {
private:
	// Just preventing easy modification of the json file.
	PackedByteArray FILE_KEY;
	const String DICT_REFUSED = "refused_to_collect_data";
	const String DICT_STOP_GROUP = "session_end";
	const String DICT_STOP_DURATION = "duration";
	const String DICT_STOP_ID = "session_id";

	enum class OSEnum {
		Undefined = 0,
		Windows = 1,
		Linux = 2,
		macOS = 3,
		Android = 4,
		iOS = 5,
		Web = 6,
	};

	enum class OSArch {
		Undefined = 0,
		x86_32 = 1,
		x86_64 = 2,
		Arm32 = 3,
		Arm64 = 4,
		RiscV32 = 5,
		RiscV64 = 6,
		Wasm32 = 7,
		Wasm64 = 8,
		E2K = 9,
	};

	class ClientSubmitData {
	public:
		/// <example>stringstringstringstringstringst</example>
		//[Required, MinLength(32), MaxLength(32)]
		String UID;
		/// <example>A1601EF0-7289-1F2C-FFB3-6F88A7219939</example>
		//[Required, MinLength(36), MaxLength(36)]
		String ProductUID;
		/// <example>1</example>
		//[Required]
		int64_t SessionUID;

		ClientSubmitData(
				const String &p_UID,
				const String &p_ProductUID,
				const int64_t &p_SessionUID) :

				UID(p_UID),
				ProductUID(p_ProductUID),
				SessionUID(p_SessionUID) {}

		virtual Dictionary serialize() {
			return DSTGodotUtils::make_dict(
					DST_NAMEOF(UID), UID,
					DST_NAMEOF(ProductUID), ProductUID,
					DST_NAMEOF(SessionUID), SessionUID);
		}
	};

	class ClientSubmitCreatingNewRow : ClientSubmitData {
	public:
		/// <example>en</example>
		//[Required, MinLength(2), MaxLength(3)]
		String AppLanguage;

		/// <example>ru</example>
		//[MinLength(2), MaxLength(3)]
		String SystemLanguage;

		/// <example>RU</example>
		//[MinLength(2), MaxLength(3)]
		String SystemRegion;

		/// <example>Web</example>
		UsageTimeReporter::OSEnum OS;

		/// <example>11 Bluestone</example>
		//[MaxLength(64)]
		String OSVersion;

		/// <example>E2K</example>
		UsageTimeReporter::OSArch OSArch;

		ClientSubmitCreatingNewRow(
				const String &p_UID,
				const String &p_ProductUID,
				const int64_t &p_SessionUID,

				const String &p_AppLanguage,
				const String &p_SystemLanguage,
				const String &p_SystemRegion,
				const UsageTimeReporter::OSEnum &p_OS,
				const String &p_OSVersion,
				const UsageTimeReporter::OSArch &p_OSArch) :

				ClientSubmitData(p_UID, p_ProductUID, p_SessionUID),
				AppLanguage(p_AppLanguage),
				SystemLanguage(p_SystemLanguage),
				SystemRegion(p_SystemRegion),
				OS(p_OS),
				OSVersion(p_OSVersion),
				OSArch(p_OSArch) {}

		virtual Dictionary serialize() override {
			Dictionary dict = ClientSubmitData::serialize();
			dict.merge(DSTGodotUtils::make_dict(
					DST_NAMEOF(AppLanguage), AppLanguage,
					DST_NAMEOF(SystemLanguage), SystemLanguage,
					DST_NAMEOF(SystemRegion), SystemRegion,
					DST_NAMEOF(OS), (int)OS,
					DST_NAMEOF(OSVersion), OSVersion,
					DST_NAMEOF(OSArch), (int)OSArch));
			return dict;
		}
	};

	class ClientSubmitDataDeviceAndProduct : ClientSubmitCreatingNewRow {
	public:
		// Product
		/// <example>1.1.1-beta</example>
		//[Required, MinLength(1), MaxLength(32)]
		String Version;

		// Device
		/// <example>false</example>
		bool RefusedToCollectData;

		ClientSubmitDataDeviceAndProduct(
				const String &p_UID,
				const String &p_ProductUID,
				const int64_t &p_SessionUID,

				const String &p_AppLanguage,
				const String &p_SystemLanguage,
				const String &p_SystemRegion,
				const UsageTimeReporter::OSEnum &p_OS,
				const String &p_OSVersion,
				const UsageTimeReporter::OSArch &p_OSArch,

				const String &p_Version,
				const bool &p_RefusedToCollectData) :

				ClientSubmitCreatingNewRow(p_UID, p_ProductUID, p_SessionUID, p_AppLanguage, p_SystemLanguage, p_SystemRegion, p_OS, p_OSVersion, p_OSArch),
				Version(p_Version),
				RefusedToCollectData(p_RefusedToCollectData) {}

		virtual Dictionary serialize() override {
			Dictionary dict = ClientSubmitCreatingNewRow::serialize();
			dict.merge(DSTGodotUtils::make_dict(
					DST_NAMEOF(Version), Version,
					DST_NAMEOF(RefusedToCollectData), RefusedToCollectData));
			return dict;
		}
	};

	class SubmitStopDurationTime : ClientSubmitData {
	public:
		/// <example>10</example>
		//[Required]
		int SessionDuration;

		SubmitStopDurationTime(
				const String &p_UID,
				const String &p_ProductUID,
				const int64_t &p_SessionUID,
				const int &p_SessionDuration) :

				ClientSubmitData(p_UID, p_ProductUID, p_SessionUID),
				SessionDuration(p_SessionDuration) {}

		virtual Dictionary serialize() override {
			Dictionary dict = ClientSubmitData::serialize();
			dict.merge(DSTGodotUtils::make_dict(
					DST_NAMEOF(SessionDuration), SessionDuration));
			return dict;
		}
	};

	struct RequestData {
	public:
		String url;
		String body;
		PackedStringArray headers;

		RequestData(const String &p_url, const String &p_body, const PackedStringArray &p_headers) :
				url(p_url),
				body(p_body),
				headers(p_headers) {};
	};

	struct RequestResult {
		HTTPClient::ResponseCode code;
		String body;
		Dictionary headers;
		String request_url;
		PackedStringArray request_headers;
		String request_body;

		RequestResult(
				HTTPClient::ResponseCode p_code,
				String p_body = "",
				Dictionary p_headers = Dictionary(),
				String p_request_url = "",
				PackedStringArray p_request_headers = PackedStringArray(),
				String p_request_body = "") :
				code(p_code),
				body(p_body),
				headers(p_headers),
				request_url(p_request_url),
				request_headers(p_request_headers),
				request_body(p_request_body) {}
	};

	struct LangRegion {
	public:
		String lang;
		String region;
		LangRegion(const String &p_lang, const String &p_region) :
				lang(p_lang),
				region(p_region) {};
	};

	std::jthread http_thread;
	int64_t current_session_uid;

	String library_version;

	PackedStringArray default_request_headers;

	String user_uid_file;

	String user_info_file;
	String user_info_file_dd3d_old;

	String app_id;
	String app_name;
	String telemetry_domain;
	String start_address;
	String stop_address;

	String accept_type_header;
	String api_version_header;
	String body_type_header;

	String s_root_addon_setting;
	String s_telemetry_setting;
	bool is_telemetry_refused = false;
	bool is_need_to_send_new_data = true;

	String generate_uid();

	String read_user_uid();
	bool write_user_uid(String uid);
	Dictionary read_user_info();
	bool write_user_info(Dictionary data, String custom_path = "");

	OSEnum get_os();
	String get_os_version();
	OSArch get_arch();
	String get_editor_language();
	LangRegion get_system_language();

	void request_completed(RequestResult result);
	void submit_data();
	void send_data(std::vector<RequestData> &queue);
	void save_data_on_closing();

	void _setup_settings();
	void _connection_body(const RequestData data, std::stop_token &token);
	RequestResult _request_body(Ref<HTTPClient> http, const RequestData &data, std::stop_token &token);

public:
	UsageTimeReporter(String app_name, String app_id, String library_version, String root_settings, String host_url, String config_file_name);
	~UsageTimeReporter();
};

// Class name fix
#define GDCLASS_CUSTOM(x, y) GDCLASS(x, y)

class UsageTimeReporterGodotObj : public Object {
	GDCLASS_CUSTOM(UsageTimeReporterGodotObj, Object)
	std::unique_ptr<UsageTimeReporter> usage_reporter;
	String app_name;
	String app_id;
	String library_version;
	String root_settings;
	String host_url;
	String config_file_name;

protected:
	static void _bind_methods();
	void _deferred_call();

public:
	UsageTimeReporterGodotObj() {};
	UsageTimeReporterGodotObj(String app_name, String app_id, String library_version, String root_settings, String host_url, String config_file_name);
};
#undef GDCLASS_CUSTOM

#define DEFINE_TELEMETRY_OBJECT_ID(obj_id_name) uint64_t obj_id_name = 0
#define INIT_EDITOR_TELEMETRY_OBJECT(obj_id_name, app_name, app_id, library_version, root_settings, host_url, config_file_name)                                      \
	if (Engine::get_singleton()->is_editor_hint()) {                                                                                                                 \
		ClassDB::register_class<UsageTimeReporterGodotObj>();                                                                                                        \
		UsageTimeReporterGodotObj *usage_reporter = memnew(UsageTimeReporterGodotObj(app_name, app_id, library_version, root_settings, host_url, config_file_name)); \
		obj_id_name = usage_reporter->get_instance_id();                                                                                                             \
	}
#define DELETE_EDITOR_TELEMETRY_OBJECT(obj_id_name)                                                                                  \
	{                                                                                                                                \
		UsageTimeReporterGodotObj *usage_reporter = Object::cast_to<UsageTimeReporterGodotObj>(ObjectDB::get_instance(obj_id_name)); \
		if (Engine::get_singleton()->is_editor_hint() && usage_reporter) {                                                           \
			memdelete(usage_reporter);                                                                                               \
			obj_id_name = 0;                                                                                                         \
		}                                                                                                                            \
	}
#endif