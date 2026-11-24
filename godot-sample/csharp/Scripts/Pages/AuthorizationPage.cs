using System.Collections.Generic;
using System.Linq;
using Godot;
using TwitchSDK;

namespace TwitchSample
{
    /// <summary>
    /// Enter a Client ID, choose scopes, and log in with Twitch's device code flow.
    /// </summary>
    public partial class AuthorizationPage : SamplePage
    {
        // Every scope the other pages need. Drops and raid events need no scope.
        static readonly (string Scope, string UsedFor)[] Scopes =
        {
            ("channel:manage:polls", "Polls page"),
            ("channel:manage:predictions", "Predictions page"),
            ("channel:manage:broadcast", "Edit channel info, create stream markers"),
            ("clips:edit", "Create clips"),
            ("channel:manage:redemptions", "Channel point rewards and redemption events"),
            ("user:read:subscriptions", "Check whether you subscribe to a channel"),
            ("channel:read:subscriptions", "Subscription events"),
            ("moderator:read:followers", "Follow events"),
            ("bits:read", "Cheer events and the Bits leaderboard"),
            ("channel:read:hype_train", "Hype Train events"),
            ("user:read:email", "Email address in your user info"),
        };

        LineEdit clientIdEdit;
        Label clientIdChanged;
        readonly List<CheckBox> scopeChecks = new List<CheckBox>();
        Label statusValue;
        VBoxContainer loggedOutBox;
        Control codeBox;
        Label codeLabel;
        Label activationLabel;
        VBoxContainer loggedInBox;
        Label grantedValue;
        VBoxContainer userBox;
        string userCode = "";
        string activationUri = "";

        public override string Title => "Authorization";

        protected override void Build()
        {
            BuildClientId();
            BuildScopes();
            BuildLogin();
            App.AuthStateChanged += UpdateLogin;
            UpdateLogin();
        }

        public override void _ExitTree()
        {
            if (App != null)
                App.AuthStateChanged -= UpdateLogin;
        }

        public override void _Process(double delta)
        {
            base._Process(delta);
            var typed = clientIdEdit.Text.Trim();
            clientIdChanged.Visible = App.AppliedClientId.Length > 0 && typed.Length > 0 && typed != App.AppliedClientId;
            statusValue.Text = App.StatusText();
        }

        // ------------------------------------------------------------------ 1. Client ID

        void BuildClientId()
        {
            SampleUI.AddSection(this, "1. Client ID",
                "Register an application in the Twitch developer console. Set the OAuth Redirect URL to " +
                "http://localhost and the Client Type to Public (the device code login used here needs a " +
                "public client). Then paste its Client ID below.");

            clientIdEdit = SampleUI.AddTextRow(this, "Client ID", App.GetSavedClientId(), null, "Paste your Client ID here");
            var buttons = SampleUI.AddIndented(this);
            App.WatchButton(SampleUI.AddButton(buttons, "Use this Client ID", () => App.ApplyClientId(clientIdEdit.Text)),
                () => clientIdEdit.Text.Trim().Length > 0);
            SampleUI.AddButton(buttons, "Open the Twitch developer console", () => OS.ShellOpen("https://dev.twitch.tv/console/apps"));

            clientIdChanged = SampleUI.LevelLabel("The Client ID changed. Press Use this Client ID to switch to it.", SampleLogLevel.Warning);
            SampleUI.AddIndented(this).AddChild(clientIdChanged);

            var proxyText = Twitch.UseEventSubProxy
                ? "On: events come from a local EventSubProxy (development only). Change it in Project Settings > Twitch > Debug, then restart."
                : "Off: events come from Twitch. To test with a local EventSubProxy, enable it in Project Settings > Twitch > Debug, then restart.";
            SampleUI.AddRow(this, "EventSubProxy").AddChild(SampleUI.Muted(proxyText));
        }

        // ------------------------------------------------------------------ 2. Scopes

        void BuildScopes()
        {
            SampleUI.AddSection(this, "2. Scopes",
                "Permissions requested when you log in. To change them after logging in, log out and log in again.");
            foreach (var (scope, usedFor) in Scopes)
            {
                var row = new HBoxContainer();
                var check = new CheckBox { Text = scope, ButtonPressed = true, CustomMinimumSize = new Vector2(SampleUI.LabelWidth + 80, 0) };
                row.AddChild(check);
                row.AddChild(SampleUI.Muted(usedFor));
                AddChild(row);
                scopeChecks.Add(check);
            }
            var buttons = new HBoxContainer();
            AddChild(buttons);
            SampleUI.AddButton(buttons, "Select all", () => scopeChecks.ForEach(c => c.ButtonPressed = true));
            SampleUI.AddButton(buttons, "Select none", () => scopeChecks.ForEach(c => c.ButtonPressed = false));
        }

        // ------------------------------------------------------------------ 3. Log in

        void BuildLogin()
        {
            SampleUI.AddSection(this, "3. Log in");
            statusValue = SampleUI.AddKeyValue(this, "Status", "");

            // Logged out: the Log in button and, once requested, the device code.
            loggedOutBox = new VBoxContainer();
            AddChild(loggedOutBox);
            App.ActionButton(SampleUI.AddIndented(loggedOutBox), "Log in with Twitch", LogIn, null, requiresLogin: false);

            var code = SampleUI.AddPanel(loggedOutBox);
            codeBox = (Control)code.GetParent();
            code.AddChild(SampleUI.MakeLabel("Open the activation page and enter this code:"));
            codeLabel = SampleUI.MakeLabel("", Colors.White, 30, true);
            code.AddChild(codeLabel);
            activationLabel = SampleUI.MakeLabel("", SampleUI.TextColor, 0, false, true);
            code.AddChild(activationLabel);
            var codeButtons = new HBoxContainer();
            code.AddChild(codeButtons);
            SampleUI.AddButton(codeButtons, "Open activation page", () => OS.ShellOpen(activationUri));
            SampleUI.AddButton(codeButtons, "Copy code", () =>
            {
                DisplayServer.ClipboardSet(userCode);
                App.Info("Code copied to the clipboard.");
            });
            code.AddChild(SampleUI.Muted("This page updates by itself once you approve the login on Twitch."));

            // Logged in: granted scopes, user info, log out.
            loggedInBox = new VBoxContainer();
            AddChild(loggedInBox);
            grantedValue = SampleUI.AddKeyValue(loggedInBox, "Granted scopes", "");
            var buttons = SampleUI.AddIndented(loggedInBox);
            App.ActionButton(buttons, "Get my user info", GetMyUserInfo);
            App.ActionButton(buttons, "Log out", LogOut);
            userBox = new VBoxContainer();
            loggedInBox.AddChild(userBox);
        }

        void UpdateLogin()
        {
            var loggedIn = App.IsLoggedIn;
            loggedOutBox.Visible = !loggedIn;
            loggedInBox.Visible = loggedIn;
            if (loggedIn)
                userCode = "";
            codeBox.Visible = userCode.Length > 0;
            codeLabel.Text = userCode;
            activationLabel.Text = "Activation page: " + activationUri;
            grantedValue.Text = App.GrantedScopes.Length == 0 ? "(none)" : string.Join(", ", App.GrantedScopes);
        }

        void LogIn()
        {
            var requested = scopeChecks.Where(c => c.ButtonPressed).Select(c => c.Text).ToArray();
            App.Run("Log in with Twitch", async () =>
            {
                var info = await Twitch.GetAuthenticationInfo(Enumerable.Empty<OAuthScope>(), requested);
                if (info == null)
                {
                    App.Info("Already logged in.");
                    return;
                }
                userCode = info.UserCode;
                activationUri = info.Uri;
                UpdateLogin();
                App.Info($"Enter code {info.UserCode} at {info.Uri} to finish logging in.");
                App.RefreshAuthState();
                OS.ShellOpen(info.Uri);
            });
        }

        void GetMyUserInfo()
        {
            App.Run("Get my user info", async () =>
            {
                var me = await Twitch.GetMyUserInfo();
                SampleUI.Clear(userBox);
                var box = SampleUI.AddPanel(userBox);
                SampleUI.AddKeyValue(box, "Display name", me.DisplayName);
                SampleUI.AddKeyValue(box, "Login", me.LoginName);
                SampleUI.AddKeyValue(box, "User ID", me.ChannelId);
                SampleUI.AddKeyValue(box, "Broadcaster type", string.IsNullOrEmpty(me.BroadcasterType) ? "(none)" : me.BroadcasterType);
                SampleUI.AddKeyValue(box, "Email", me.Email);
                App.Success($"You are logged in as {me.DisplayName}.");
            });
        }

        void LogOut()
        {
            App.Run("Log out", async () =>
            {
                await Twitch.LogOut();
                SampleUI.Clear(userBox);
                App.Success("Logged out. The saved token was removed.");
                App.RefreshAuthState();
            });
        }
    }
}
