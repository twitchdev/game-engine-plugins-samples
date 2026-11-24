using System;
using System.Linq;
using TwitchSDK;
using TwitchSDK.Interop;
using UnityEngine;

namespace TwitchSample
{
    /// <summary>
    /// Enter a Client ID, choose scopes, and log in with Twitch's device code flow.
    /// </summary>
    public sealed class AuthorizationPage : SamplePage
    {
        const string ClientIdPrefKey = "TwitchSample.ClientId";
        const string ProxyPrefKey = "TwitchSample.UseEventSubProxy";
        const float StatePollSeconds = 1f;

        sealed class ScopeOption
        {
            public readonly string Scope;
            public readonly string UsedFor;
            public bool Enabled = true;

            public ScopeOption(string scope, string usedFor)
            {
                Scope = scope;
                UsedFor = usedFor;
            }
        }

        // Every scope the other pages need. Drops and raid events need no scope.
        readonly ScopeOption[] scopes =
        {
            new ScopeOption("channel:manage:polls", "Polls page"),
            new ScopeOption("channel:manage:predictions", "Predictions page"),
            new ScopeOption("channel:manage:broadcast", "Edit channel info, create stream markers"),
            new ScopeOption("clips:edit", "Create clips"),
            new ScopeOption("channel:manage:redemptions", "Channel point rewards and redemption events"),
            new ScopeOption("user:read:subscriptions", "Check whether you subscribe to a channel"),
            new ScopeOption("channel:read:subscriptions", "Subscription events"),
            new ScopeOption("moderator:read:followers", "Follow events"),
            new ScopeOption("bits:read", "Cheer events and the Bits leaderboard"),
            new ScopeOption("channel:read:hype_train", "Hype Train events"),
            new ScopeOption("user:read:email", "Email address in your user info"),
        };

        string clientId;
        bool useEventSubProxy;
        bool clientIdChanged;

        AuthenticationInfo authInfo;
        UserInfo me;

        GameTask<AuthState> pendingState;
        float nextStatePoll;
        bool loggedStateError;

        public override string Title => "Authorization";

        public AuthorizationPage()
        {
            clientId = PlayerPrefs.GetString(ClientIdPrefKey, string.Empty);
            useEventSubProxy = PlayerPrefs.GetInt(ProxyPrefKey, 0) == 1;
        }

        public override void ReleaseApiResources()
        {
            pendingState = null;
            authInfo = null;
            me = null;
            loggedStateError = false;
        }

        /// <summary>Polls the auth state about once a second so the status stays current on every page.</summary>
        public override void Tick()
        {
            if (Api == null)
                return;

            if (pendingState != null)
            {
                if (!pendingState.IsCompleted)
                    return;

                var task = pendingState.Task;
                pendingState = null;
                if (task.IsFaulted)
                {
                    if (!loggedStateError)
                        SampleLog.Error("Could not read the login state: " + TwitchSampleApp.Describe(task.Exception));
                    loggedStateError = true;
                    return;
                }

                var previous = App.AuthState?.Status;
                App.AuthState = task.Result;
                if (App.AuthState.Status == AuthStatus.LoggedIn && previous != AuthStatus.LoggedIn)
                {
                    authInfo = null;
                    if (previous != null)
                        SampleLog.Success("Logged in to Twitch.");
                    else
                        SampleLog.Info("Already logged in (using the saved token).");
                }
                return;
            }

            if (Time.unscaledTime >= nextStatePoll)
            {
                nextStatePoll = Time.unscaledTime + StatePollSeconds;
                pendingState = Api.GetAuthState();
            }
        }

        public override void Draw()
        {
            DrawClientId();
            DrawScopes();
            DrawLogin();
        }

        void DrawClientId()
        {
            SampleGUI.Section("1. Client ID",
                "Register an application in the Twitch developer console. Set the OAuth Redirect URL to " +
                "http://localhost and the Client Type to Public (the device code login used here needs a " +
                "public client). Then paste its Client ID below.");

            clientId = SampleGUI.TextField("Client ID", clientId);
            useEventSubProxy = SampleGUI.Toggle(
                "Use a local EventSubProxy instead of Twitch (development only, for testing events)", useEventSubProxy);

            GUILayout.BeginHorizontal();
            GUILayout.Space(SampleGUI.LabelWidth + 4);
            if (SampleGUI.Button(Api == null ? "Connect" : "Reconnect", !string.IsNullOrWhiteSpace(clientId)))
                SampleGUI.Defer(Connect);
            if (SampleGUI.Button("Disconnect", Api != null))
                SampleGUI.Defer(() => App.Disconnect());
            if (SampleGUI.Button("Open the Twitch developer console"))
                Application.OpenURL("https://dev.twitch.tv/console/apps");
            GUILayout.EndHorizontal();

            // Decide in the Layout event so typing doesn't add the label mid-event.
            if (Event.current.type == EventType.Layout)
                clientIdChanged = Api != null && Api.ClientId != clientId.Trim();
            if (clientIdChanged)
                GUILayout.Label("The Client ID changed. Press Reconnect to use it.", SampleGUI.StyleFor(SampleLogLevel.Warning));
        }

        void Connect()
        {
            var id = clientId.Trim();
            PlayerPrefs.SetString(ClientIdPrefKey, id);
            PlayerPrefs.SetInt(ProxyPrefKey, useEventSubProxy ? 1 : 0);
            PlayerPrefs.Save();

            if (App.Connect(id, useEventSubProxy))
                nextStatePoll = 0f; // read the login state right away
        }

        void DrawScopes()
        {
            SampleGUI.Section("2. Scopes",
                "Permissions requested when you log in. To change them after logging in, log out and log in again.");

            foreach (var option in scopes)
            {
                GUILayout.BeginHorizontal();
                option.Enabled = GUILayout.Toggle(option.Enabled, "  " + option.Scope, GUILayout.Width(SampleGUI.LabelWidth + 80));
                GUILayout.Label(option.UsedFor, SampleGUI.MutedStyle);
                GUILayout.EndHorizontal();
            }

            GUILayout.BeginHorizontal();
            if (SampleGUI.Button("Select all"))
                foreach (var option in scopes) option.Enabled = true;
            if (SampleGUI.Button("Select none"))
                foreach (var option in scopes) option.Enabled = false;
            GUILayout.EndHorizontal();
        }

        void DrawLogin()
        {
            SampleGUI.Section("3. Log in");

            if (Api == null)
            {
                GUILayout.Label("Connect with your Client ID first.", SampleGUI.MutedStyle);
                return;
            }

            var state = App.AuthState;
            SampleGUI.KeyValue("Status", state == null ? "Checking ..." : state.Status.ToString());

            if (App.IsLoggedIn)
                DrawLoggedIn(state);
            else
                DrawLoggedOut();
        }

        void DrawLoggedOut()
        {
            if (SampleGUI.ActionButton(App, "Log in with Twitch", requiresLogin: false))
                LogIn();

            if (authInfo == null)
                return;

            GUILayout.Space(8);
            GUILayout.BeginVertical(GUI.skin.box);
            GUILayout.Label("Open the activation page and enter this code:", SampleGUI.WrapStyle);
            GUILayout.Label(authInfo.UserCode, SampleGUI.BigStyle);
            SampleGUI.KeyValue("Activation page", authInfo.Uri);
            GUILayout.BeginHorizontal();
            if (SampleGUI.Button("Open activation page"))
                Application.OpenURL(authInfo.Uri);
            if (SampleGUI.Button("Copy code"))
            {
                GUIUtility.systemCopyBuffer = authInfo.UserCode;
                SampleLog.Info("Code copied to the clipboard.");
            }
            GUILayout.EndHorizontal();
            GUILayout.Label("This page updates by itself once you approve the login on Twitch.", SampleGUI.MutedStyle);
            GUILayout.EndVertical();
        }

        void LogIn()
        {
            var requested = scopes.Where(x => x.Enabled).Select(x => new TwitchOAuthScope(x.Scope)).ToArray();
            App.Run("Log in with Twitch", async () =>
            {
                var info = await Api.GetAuthenticationInfo(requested);
                if (info == null)
                {
                    SampleLog.Info("Already logged in.");
                    return;
                }

                authInfo = info;
                SampleLog.Info($"Enter code {info.UserCode} at {info.Uri} to finish logging in.");
                Application.OpenURL(info.Uri);
            });
        }

        void DrawLoggedIn(AuthState state)
        {
            var granted = state.Scopes == null || state.Scopes.Length == 0 ? "(none)" : string.Join(", ", state.Scopes);
            SampleGUI.KeyValue("Granted scopes", granted);

            GUILayout.BeginHorizontal();
            GUILayout.Space(SampleGUI.LabelWidth + 4);
            if (SampleGUI.ActionButton(App, "Get my user info"))
            {
                App.Run("Get my user info", async () =>
                {
                    me = await Api.GetMyUserInfo();
                    SampleLog.Success($"You are logged in as {me.DisplayName}.");
                });
            }
            if (SampleGUI.ActionButton(App, "Log out"))
            {
                App.Run("Log out", async () =>
                {
                    await Api.LogOut();
                    me = null;
                    SampleLog.Success("Logged out. The saved token was removed.");
                    nextStatePoll = 0f;
                });
            }
            GUILayout.EndHorizontal();

            if (me != null)
            {
                GUILayout.Space(6);
                GUILayout.BeginVertical(GUI.skin.box);
                SampleGUI.KeyValue("Display name", me.DisplayName);
                SampleGUI.KeyValue("Login", me.LoginName);
                SampleGUI.KeyValue("User ID", me.ChannelId);
                SampleGUI.KeyValue("Broadcaster type", string.IsNullOrEmpty(me.BroadcasterType) ? "(none)" : me.BroadcasterType);
                SampleGUI.KeyValue("Email", me.Email);
                GUILayout.EndVertical();
            }
        }
    }
}
