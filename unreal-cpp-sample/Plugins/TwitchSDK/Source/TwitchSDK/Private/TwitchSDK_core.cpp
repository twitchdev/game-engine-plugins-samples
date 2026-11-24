#include "TwitchSDK_core.h"

#include "TwitchSDK.h"
#include <exception>
#include <cstdint>
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"

#include "twitchsdk_json.hpp"

namespace TwitchSDK {
	class Json {
		TSharedPtr<FJsonValue> UnrealJson;
		explicit Json(TSharedPtr<FJsonValue> j) : UnrealJson(j)
		{
		}
	public:
		Json() : UnrealJson(MakeShared<FJsonValueObject>(MakeShared<FJsonObject>())) {}
		explicit Json(string_view s) : UnrealJson(MakeShared<FJsonValueString>(ToFString(s))) {}
		explicit Json(std::string_view s) : UnrealJson(MakeShared<FJsonValueString>(FString(s.size(), s.data()))) {}
		explicit Json(const twitchsdkchar* s) : Json(string_view(s)) {}
		explicit Json(const char* s) : Json(std::string_view(s)) {}
		explicit Json(int64_t i) : UnrealJson(MakeShared<FJsonValueNumber>((double)i)) {}
		explicit Json(int32_t i) : UnrealJson(MakeShared<FJsonValueNumber>((double)i)) {}
		explicit Json(bool i) : UnrealJson(MakeShared<FJsonValueBoolean>(i)) {}

		static Json Parse(string_view s) {
			TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(ToFString(s));
			TSharedPtr<FJsonValue> json;
			FJsonSerializer::Deserialize(reader, json);
			if (!json.IsValid())
				throw std::runtime_error("invalid json");
			return Json(json);
		}
		Json at(const std::string& path) const
		{
			const TSharedPtr<FJsonObject>* self;
			if (!UnrealJson->TryGetObject(self))
				throw JsonMissingKeyError(path);
			auto out = (*self)->TryGetField(FString(path.c_str()));
			if (!out.IsValid())
				throw JsonMissingKeyError(path);
			return Json(out);
		}
		Json at(std::size_t i) const
		{
			const TArray<TSharedPtr<FJsonValue>>* self;
			if (!UnrealJson->TryGetArray(self))
				throw JsonMissingKeyError(std::to_string(i));

			if (!self->IsValidIndex(i))
				throw JsonMissingKeyError(std::to_string(i));
			return Json((*self)[i]);
		}
		std::string to_std_string() const
		{
			FString fs;
			if (!UnrealJson->TryGetString(fs))
				throw JsonBadValueError("string");

			return std::string(TCHAR_TO_UTF8(*fs));
		}
		int64_t to_int() const {
			int64 i;
			if (!UnrealJson->TryGetNumber(i))
				throw JsonBadValueError("int");
			return static_cast<int64_t>(i);
		}
		bool to_bool() const {
			bool b;
			if (!UnrealJson->TryGetBool(b))
				throw JsonBadValueError("bool");
			return b;
		}
		string to_string() const {
			FString fs;
			if (!UnrealJson->TryGetString(fs))
				throw JsonBadValueError("string");

			return string(FromFString(*fs));
		}

		size_t array_size() const {
			const TArray<TSharedPtr<FJsonValue>>* self;
			if (!UnrealJson->TryGetArray(self))
				throw JsonBadValueError("array");

			return self->Num();
		}

		bool is_null() const {
			return UnrealJson->IsNull();
		}

		void set_property(const std::string& path, Json value)
		{
			const TSharedPtr<FJsonObject>* self;
			if (!UnrealJson->TryGetObject(self))
				throw JsonBadValueError("object");
			self->Get()->SetField(FString(path.c_str()), value.UnrealJson);
		}
		void set_array(const std::string& path, std::vector<Json>&& data)
		{
			const TSharedPtr<FJsonObject>* self;
			if (!UnrealJson->TryGetObject(self))
				throw JsonBadValueError("object");

			TArray<TSharedPtr<FJsonValue>> arr;
			arr.Reserve(data.size());
			for (auto&& j : data) {
				arr.Push(std::move(j.UnrealJson));
			}
			self->Get()->SetArrayField(FString(path.c_str()), arr);
		}

		string serialize() const
		{
			const TSharedPtr<FJsonObject>* self;
			if (!UnrealJson->TryGetObject(self))
				throw std::runtime_error("unreal does not support serializing non-object toplevel json values");

			FString out;
			auto writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&out, 0);
			FJsonSerializer::Serialize(self->ToSharedRef(), writer);
			return string(FromFString(out));
		}
	};
	using JsonRef = Json;
}

TwitchSDK::string_holder TwitchSDK::from_utf8(std::string_view s)
{
	return string_holder(TwitchSDK::string(UTF8_TO_TCHAR(s.data())));
}

TwitchSDK::stdstring_holder TwitchSDK::to_utf8(string_view s)
{
	return stdstring_holder(std::string(TCHAR_TO_UTF8(s.data())));
}


#include "urlencode.cpp"
#include "twitchsdk.cpp"
#include "twitchsdk_auth.cpp"
#include "twitchsdk_channelpoints.cpp"
#include "twitchsdk_drops.cpp"
#include "twitchsdk_es.cpp"
#include "twitchsdk_misc.cpp"
#include "twitchsdk_polls.cpp"
#include "twitchsdk_predictions.cpp"
#include "twitchsdk_streaminfo.cpp"
#include "twitchsdk_userinfo.cpp"
#include "twitchsdk_unreal_build_version.h"
#include "version.cpp.in"
