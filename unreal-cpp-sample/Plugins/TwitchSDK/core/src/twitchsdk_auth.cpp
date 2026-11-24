#include "twitchsdk_impl.hpp"


#define AUTH_DATA_FILE STR("twitch-twitchsdk-authorization.json"sv)

// Prior to the R66 -> TwitchSDK rename, OAuth tokens were persisted under this
// filename. We still read it as a fallback so that users who authenticated with
// an older build of the plugin are not silently logged out after upgrading. When
// found, the tokens are migrated into AUTH_DATA_FILE on the next token write.
#define AUTH_DATA_FILE_LEGACY STR("twitch-r66-authorization.json"sv)

// Attempt to parse auth tokens out of a data file body. Returns true on success.
// Any parse/format error is logged and swallowed (returns false) so startup can
// continue with the user simply logged out. sourceDescription is a narrow (UTF-8)
// string literal used only for logging.
static bool TryLoadAuthTokensFromData(
	std::shared_ptr<TwitchSDK::TwitchSDKApiImpl> self,
	string_view data,
	const char* sourceDescription)
{
	using namespace TwitchSDK;
	if (data.length() == 0)
		return false;

	try {
		auto j = Json::Parse(data);
		TwitchSDKApiImpl::TryParseAuthTokens(self, j);
		Log(self, LogLevel::Debug, "Using auth info from %s"sv, sourceDescription);
		return true;
	}
	catch (const std::exception& e)
	{
		Log(self, LogLevel::Debug, "Error while trying to parse %s: %s"sv, sourceDescription, e.what());
		return false;
	}
}

void TwitchSDK::TwitchSDKApiImpl::LoadLegacyAuthTokens(Self self) {
	// Fallback path: the current token file was missing or empty. Try the
	// pre-rename filename and, if it contains valid tokens, migrate them into
	// the current file so future reads/writes use the new name.
	self->PAL->ReadFile(
		AUTH_DATA_FILE_LEGACY,
		[self](string_view data)
		{
			std::lock_guard<std::recursive_mutex> lock(self->Mutex);

			if (TryLoadAuthTokensFromData(self, data, "legacy auth data file"))
			{
				// migrate the tokens to the new filename (best-effort)
				self->PAL->WriteFile(
					AUTH_DATA_FILE,
					data,
					[self]()
					{
						Log(self, LogLevel::Debug, "Migrated legacy auth token file to new filename"sv);
					},
					[self](const std::exception& e)
					{
						Log(self, LogLevel::Warning, "Failed to migrate legacy auth token file: %s"sv, e.what());
					}
				);
			}

			InitDone(self);
		},
		[self](const std::exception& e)
		{
			// no legacy file either: nothing to migrate, start logged out
			Log(self, LogLevel::Debug, "No legacy auth data file to read: %s"sv, e.what());
			InitDone(self);
		}
	);
}

void TwitchSDK::TwitchSDKApiImpl::LoadAuthTokens(Self self) {
	self->PAL->ReadFile(
		AUTH_DATA_FILE,
		[self](string_view data)
		{
			std::lock_guard<std::recursive_mutex> lock(self->Mutex);

			if (TryLoadAuthTokensFromData(self, data, "existing data file"))
			{
				InitDone(self);
			}
			else
			{
				// current file missing/empty/invalid: try the legacy filename
				LoadLegacyAuthTokens(self);
			}
		},
		[self](const std::exception& e)
		{
			std::lock_guard<std::recursive_mutex> lock(self->Mutex);
			Log(self, LogLevel::Debug, "Error while trying to read existing data file: %s"sv, e.what());
			// current file could not be read: try the legacy filename
			LoadLegacyAuthTokens(self);
		}
	);
}

void TwitchSDKApiImpl::TryParseAuthTokens(Self self, const Json& j)
{
	AuthorizationResult ar;
	ar.AccessToken = j.at("access_token").to_string();
	ar.RefreshToken = j.at("refresh_token").to_string();
	auto scopes = j.at("scope");
	auto num_scopes = scopes.array_size();
	for (size_t i = 0; i < num_scopes; i++)
	{
		ar.Scopes.push_back(scopes.at(i).to_string());
	}
	self->AuthResult.Resolve(std::move(ar));
	self->MyAuthStatus = AuthStatus::LoggedIn;
}

void TwitchSDKApiImpl::RefreshAccessToken(Self self)
{
	std::lock_guard<std::recursive_mutex> lock(self->Mutex);
	if (!self->AuthResult.IsDone()) return; // race probably

	string refresh = self->AuthResult.Unwrap().RefreshToken;
	self->AuthResult.Invalidate();

	string uri = STR("https://id.twitch.tv/oauth2/token?grant_type=refresh_token"s);
	uri += STR("&refresh_token="sv);
	uri += refresh;
	uri += STR("&client_id="sv);
	uri += self->ClientId;
	self->PAL->WebRequest(
		HttpMethod::Post,
		uri,
		STR("text/plain"sv),
		string_view(),
		string_view(),
		string_view(),
		[self](const WebRequestResult& r)
		{
			std::lock_guard<std::recursive_mutex> lock(self->Mutex);

			if (r.HttpStatus == 400)
			{
				// probably "Invalid OAuth token" meaning that we have been logged out
				Deauthenticate(self, true);
				return;
			}
			TwitchSDKApiImpl::TryParseAuthTokens(self, Json::Parse(r.ResponseBody));

			self->PAL->WriteFile(
				AUTH_DATA_FILE,
				r.ResponseBody,
				[self]()
				{
					Log(self, LogLevel::Debug, "Auth token file successfully written!"sv);
				},
				[self](const std::exception& e)
				{
					Log(self, LogLevel::Warning, "Failed to write auth token file: %s"sv, e.what());
				}
			);
		},
		[self](const std::exception&)
		{
			std::lock_guard<std::recursive_mutex> lock(self->Mutex);
			// on error, give up and make sure that the user can reauthenticate
			// TODO: retry here
			self->AuthInfo.Invalidate();
		}
	);
}

void TwitchSDK::TwitchSDKApiImpl::Deauthenticate(Self self, bool deletePersistently) {
	std::lock_guard<std::recursive_mutex> lock(self->Mutex);
	self->MyAuthStatus = AuthStatus::LoggedOut;
	self->AuthResult.Invalidate();
	self->AuthInfo.Invalidate();
	self->CachedCustomRewardList.Invalidate();
	if (deletePersistently)
	{
		self->PAL->WriteFile(
			AUTH_DATA_FILE,
			string_view(), // overwrite with empty file
			[]() {},
			[](const std::exception&) {}
		);
	}
}

void TwitchSDKApiImpl::CheckAuthenticationState(Self self)
{
	std::lock_guard<std::recursive_mutex> lock(self->Mutex);

	string requestBody;
	requestBody += STR("client_id="sv);
	EncodeUriComponent(requestBody, self->ClientId);
	requestBody += STR("&device_code="sv);
	EncodeUriComponent(requestBody, self->AuthInfo.Unwrap().DeviceCode);

	auto reject = [self](const std::exception& e)
	{
		std::lock_guard<std::recursive_mutex> lock(self->Mutex);
		Log(self, LogLevel::Info, "Fatal error when waiting for code result: %s"sv, e.what());
		// unset AuthInfo and start over
		self->AuthInfo.Invalidate();
		self->MyAuthStatus = AuthStatus::LoggedOut;
	};
	self->PAL->WebRequest(
		HttpMethod::Post,
		STR("https://id.twitch.tv/oauth2/token?grant_type=urn:ietf:params:oauth:grant-type:device_code"sv),
		STR("application/x-www-form-urlencoded"sv),
		string_view(),
		string_view(),
		requestBody,
		[self, reject](const WebRequestResult& r)
		{
			auto j = Json::Parse(r.ResponseBody);
			std::lock_guard<std::recursive_mutex> lock(self->Mutex);

			try {
				TwitchSDKApiImpl::TryParseAuthTokens(self, j);
			}
			catch (const JsonMissingKeyError&) {
				if (j.at("message").to_std_string() == "authorization_pending"sv)
				{
					// waiting/pending
					self->PAL->Sleep(self->AuthInfo.Unwrap().Interval * 1000, [self]() { CheckAuthenticationState(self); }, reject);
					return;
				}
				else {
					throw;
				}
			}

			self->PAL->WriteFile(
				AUTH_DATA_FILE,
				r.ResponseBody,
				[self]()
				{
					Log(self, LogLevel::Debug, "Auth token file successfully written!"sv);
				},
				[self](const std::exception& e)
				{
					Log(self, LogLevel::Warning, "Failed to write auth token file: %s"sv, e.what());
				}
			);
		},
		reject
	);
}


void TwitchSDKApiImpl::GetAuthenticationInfo(string_view scopes, ResolveFn<const AuthenticationInfo&> resolve, RejectFn reject)
{
	Initialized.Await([self = shared_from_this(), scopes = string(scopes), resolve, reject](const VoidResult&) {
		TwitchSDKApiImpl::FetchAuthenticationInfo(self, scopes, resolve, reject);
	});
}

void TwitchSDKApiImpl::FetchAuthenticationInfo(Self self, string scopes, ResolveFn<const AuthenticationInfo&> resolve, RejectFn reject)
{
	std::lock_guard<std::recursive_mutex> lock(self->Mutex);

	if (self->AuthResult.IsDone())
	{
		Log(self, LogLevel::Info, "Already authenticated!"sv);
		resolve(AuthenticationInfo());
		return;
	}

	if (!self->AuthInfo.Await([r = std::move(resolve)](const AuthenticationInfoInternal& ai) { r(ai.Inner); }, std::move(reject)))
	{
		return;
	}

	Log(self, LogLevel::Debug, "Starting authorization flow"sv);
	self->MyAuthStatus = AuthStatus::Loading;

	string requestBody;
	requestBody += STR("client_id="sv);
	EncodeUriComponent(requestBody, self->ClientId);
	requestBody += STR("&scopes="sv);
	EncodeUriComponent(requestBody, scopes);

	self->PAL->WebRequest(
		HttpMethod::Post,
		STR("https://id.twitch.tv/oauth2/device"sv),
		STR("application/x-www-form-urlencoded"sv),
		string_view(),
		string_view(),
		requestBody,
		[self](const WebRequestResult& r)
		{
			std::lock_guard<std::recursive_mutex> lock(self->Mutex);

			if (r.HttpStatus != 200)
			{
				// throw in order to drop directly into the reject path below.
				// (Not `throw e;`: that throws a sliced std::exception copy and loses the message.)
				BadHttpResponse(r, [&](const std::exception& e) { throw std::runtime_error(e.what()); });
			}

			auto j = Json::Parse(r.ResponseBody);
			AuthenticationInfoInternal ai;
			ai.Inner.Uri = j.at("verification_uri").to_string();
			ai.Inner.UserCode = j.at("user_code").to_string();
			ai.DeviceCode = j.at("device_code").to_string();
			ai.Interval = (int)j.at("interval").to_int();
			self->AuthInfo.Resolve(std::move(ai));
			// We ignore expires_in as recommened at https://dev.twitch.tv/docs/authentication

			// launch the worker that periodically polls the authentication state
			// while this worker is running, ai.IsSet is true so that we will only
			// request new authinfo once the worker errors (for whatever reason)
			self->MyAuthStatus = AuthStatus::WaitingForCode;
			TwitchSDKApiImpl::CheckAuthenticationState(self);
		},
		[self](const std::exception& e)
		{
			std::lock_guard<std::recursive_mutex> lock(self->Mutex);
			self->MyAuthStatus = AuthStatus::LoggedOut;

			self->AuthInfo.Reject(e);
			// invalidate right away so that they can try again
			self->AuthInfo.Invalidate();
		}
	);
}

void TwitchSDK::TwitchSDKApiImpl::GetAuthState(ResolveFn<const AuthState&> resolve, RejectFn reject) {
	std::lock_guard<std::recursive_mutex> lock(Mutex);
	AuthState as;
	as.Status = MyAuthStatus;
	if (AuthResult.IsDone())
	{
		for (auto& scope : AuthResult.Unwrap().Scopes)
		{
			as.Scopes.emplace_back(scope);
		}
	}
	resolve(as);
}

void TwitchSDK::TwitchSDKApiImpl::LogOut(ResolveFn<void> resolve, RejectFn reject) {
	std::lock_guard<std::recursive_mutex> lock(Mutex);
	TwitchSDKApiImpl::Deauthenticate(shared_from_this(), true);
	resolve();
}
