using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using Godot;
using TwitchSDK;

namespace TwitchSample
{
    /// <summary>
    /// The sample's root: header, page tabs and the log panel. Pages use it to run API calls and log.
    /// </summary>
    /// <remarks>
    /// Everything runs on the main thread: the Twitch plugin completes its tasks from the frame loop,
    /// so every await resumes on the main thread and the UI can be updated directly afterwards.
    /// </remarks>
    public partial class SampleApp : Control
    {
        const int MaxLogEntries = 300;
        const int MaxVisibleLogEntries = 150;
        const int LogHeight = 200;
        const string SettingsPath = "user://twitch_sample.cfg";

        sealed class LogEntry
        {
            public string Time;
            public SampleLogLevel Level;
            public string Message;
        }

        sealed class TrackedButton
        {
            public Button Button;
            public string Action;
            public Func<bool> Enabled;
            public bool RequiresLogin;
        }

        /// <summary>Raised when the login status changes.</summary>
        public event Action AuthStateChanged;

        /// <summary>The latest login status, or null until it has been read.</summary>
        public AuthStatus? AuthStatus { get; private set; }
        public string[] GrantedScopes { get; private set; } = Array.Empty<string>();
        /// <summary>The Client ID the sample handed to the plugin, or "" if none yet.</summary>
        public string AppliedClientId { get; private set; } = "";
        public bool ShowCoreDebugMessages { get; set; }

        public bool IsLoggedIn => Twitch.IsInitialized && AuthStatus == TwitchSDK.AuthStatus.LoggedIn;

        readonly HashSet<string> running = new HashSet<string>();
        readonly List<TrackedButton> actionButtons = new List<TrackedButton>();
        readonly List<TrackedButton> watchedButtons = new List<TrackedButton>();
        readonly List<LogEntry> log = new List<LogEntry>();
        readonly List<SamplePage> pages = new List<SamplePage>();
        RichTextLabel logView;
        Label statusLabel;
        TabContainer tabs;
        bool readingAuthState;

        public override void _Ready()
        {
            Theme = SampleUI.MakeTheme();
            Build();

            Twitch.LogMessage += OnCoreLog;
            Info($"TwitchSDK plugin {Twitch.PluginVersion} loaded (core library {Twitch.Version}).");

            // The plugin starts once it has a Client ID. Use the one saved by an earlier session, if any,
            // so its saved login keeps working.
            var saved = GetSavedClientId();
            if (!string.IsNullOrEmpty(saved))
                ApplyClientId(saved);

            var timer = new Timer { WaitTime = 1.0, Autostart = true };
            timer.Timeout += RefreshAuthState;
            AddChild(timer);
            RefreshAuthState();
        }

        public override void _ExitTree() => Twitch.LogMessage -= OnCoreLog;

        public override void _Process(double delta)
        {
            statusLabel.Text = "Status: " + StatusText();

            var started = Twitch.IsInitialized;
            var loggedIn = IsLoggedIn;
            actionButtons.RemoveAll(b => !IsInstanceValid(b.Button));
            foreach (var b in actionButtons)
            {
                var isRunning = IsRunning(b.Action);
                var allowed = b.Enabled == null || b.Enabled();
                b.Button.Disabled = !(started && (loggedIn || !b.RequiresLogin) && allowed && !isRunning);
                b.Button.Text = isRunning ? b.Action + " ..." : b.Action;
            }
            watchedButtons.RemoveAll(b => !IsInstanceValid(b.Button));
            foreach (var b in watchedButtons)
                b.Button.Disabled = !b.Enabled();
        }

        // ------------------------------------------------------------------ layout

        void Build()
        {
            var background = new Panel();
            background.AddThemeStyleboxOverride("panel", new StyleBoxFlat { BgColor = SampleUI.Background });
            background.SetAnchorsPreset(LayoutPreset.FullRect);
            AddChild(background);

            var margin = new MarginContainer();
            margin.SetAnchorsPreset(LayoutPreset.FullRect);
            foreach (var side in new[] { "left", "top", "right", "bottom" })
                margin.AddThemeConstantOverride("margin_" + side, 12);
            AddChild(margin);

            var root = new VBoxContainer();
            root.AddThemeConstantOverride("separation", 8);
            margin.AddChild(root);

            // Header: title on the left, versions and status on the right.
            var header = new HBoxContainer();
            var title = SampleUI.MakeLabel("Twitch SDK Godot Sample (C#)", SampleUI.Accent, 22, true);
            title.SizeFlagsHorizontal = SizeFlags.ExpandFill;
            header.AddChild(title);
            var versions = new VBoxContainer();
            var plugin = SampleUI.MakeLabel($"TwitchSDK plugin {Twitch.PluginVersion}, core library {Twitch.Version}", SampleUI.MutedColor, 13);
            plugin.HorizontalAlignment = HorizontalAlignment.Right;
            versions.AddChild(plugin);
            statusLabel = SampleUI.MakeLabel("", SampleUI.MutedColor, 13);
            statusLabel.HorizontalAlignment = HorizontalAlignment.Right;
            versions.AddChild(statusLabel);
            header.AddChild(versions);
            root.AddChild(header);

            // Pages, each in its own scroll area.
            tabs = new TabContainer { SizeFlagsVertical = SizeFlags.ExpandFill };
            root.AddChild(tabs);
            foreach (var page in new SamplePage[] { new AuthorizationPage(), new PollsPage(), new PredictionsPage(), new DropsPage(), new GeneralPage() })
            {
                var scroll = new ScrollContainer { Name = page.Title, HorizontalScrollMode = ScrollContainer.ScrollMode.Disabled };
                var pageMargin = new MarginContainer { SizeFlagsHorizontal = SizeFlags.ExpandFill };
                pageMargin.AddThemeConstantOverride("margin_right", 16);
                pageMargin.AddThemeConstantOverride("margin_bottom", 16);
                page.SizeFlagsHorizontal = SizeFlags.ExpandFill;
                pageMargin.AddChild(page);
                scroll.AddChild(pageMargin);
                tabs.AddChild(scroll);
                page.Setup(this);
                pages.Add(page);
            }

            // Log panel.
            var logPanel = new PanelContainer { CustomMinimumSize = new Vector2(0, LogHeight) };
            var logBox = new VBoxContainer();
            logPanel.AddChild(logBox);
            var logHeader = new HBoxContainer();
            var logTitle = SampleUI.MakeLabel("Log (newest first)", Colors.White, 15, true);
            logTitle.SizeFlagsHorizontal = SizeFlags.ExpandFill;
            logHeader.AddChild(logTitle);
            var debugToggle = new CheckBox { Text = "Show core debug messages" };
            debugToggle.Toggled += on => ShowCoreDebugMessages = on;
            logHeader.AddChild(debugToggle);
            SampleUI.AddButton(logHeader, "Clear", ClearLog);
            logBox.AddChild(logHeader);
            logView = new RichTextLabel { BbcodeEnabled = false, SelectionEnabled = true, SizeFlagsVertical = SizeFlags.ExpandFill };
            logBox.AddChild(logView);
            root.AddChild(logPanel);
        }

        // ------------------------------------------------------------------ buttons and calls

        /// <summary>
        /// A button for an API call. It shows "..." while the call with the same <paramref name="action"/>
        /// name is running, and is disabled until the library is running and, unless
        /// <paramref name="requiresLogin"/> is false, the user is logged in.
        /// </summary>
        /// <remarks>
        /// The core doesn't reject calls made while logged out. It holds them until a login happens,
        /// so allowing the click would leave the button waiting with no feedback.
        /// </remarks>
        public Button ActionButton(Control parent, string action, Action onPressed, Func<bool> enabled = null, bool requiresLogin = true)
        {
            var button = SampleUI.AddButton(parent, action, onPressed);
            actionButtons.Add(new TrackedButton { Button = button, Action = action, Enabled = enabled, RequiresLogin = requiresLogin });
            return button;
        }

        /// <summary>A plain button that is enabled only while <paramref name="enabled"/> returns true.</summary>
        public Button WatchButton(Button button, Func<bool> enabled)
        {
            watchedButtons.Add(new TrackedButton { Button = button, Enabled = enabled });
            return button;
        }

        public bool IsRunning(string action) => running.Contains(action);

        /// <summary>
        /// Runs an API call for a button: ignores repeat clicks while it's running and logs any
        /// failure with a readable message. Success messages are logged by the caller.
        /// </summary>
        public async void Run(string action, Func<Task> work)
        {
            if (running.Contains(action))
                return;
            if (!Twitch.IsInitialized)
            {
                Warning($"{action}: enter your Client ID on the Authorization page first.");
                return;
            }
            running.Add(action);
            try
            {
                await work();
            }
            catch (TwitchException e)
            {
                Error($"{action} failed: {e.Message}");
            }
            catch (Exception e)
            {
                Error($"{action} failed: {e.Message}");
            }
            finally
            {
                running.Remove(action);
            }
        }

        // ------------------------------------------------------------------ Client ID and login state

        public string GetSavedClientId()
        {
            var config = new ConfigFile();
            if (config.Load(SettingsPath) == Godot.Error.Ok)
            {
                var saved = config.GetValue("sample", "client_id", "").AsString();
                if (!string.IsNullOrEmpty(saved))
                    return saved;
            }
            // Otherwise the project setting (Project Settings > Twitch > Auth > Client Id), if set.
            return Twitch.ClientId;
        }

        public void ApplyClientId(string clientId)
        {
            var trimmed = (clientId ?? "").Trim();
            if (trimmed.Length == 0)
                return;
            var changed = AppliedClientId.Length > 0 && AppliedClientId != trimmed;

            // Starts the plugin if this is its first Client ID, or switches Client IDs (development only).
            Twitch.ClientId = trimmed;
            AppliedClientId = trimmed;

            // Remember it, so the next session starts with it and its saved login keeps working.
            var config = new ConfigFile();
            config.Load(SettingsPath);
            config.SetValue("sample", "client_id", trimmed);
            config.Save(SettingsPath);

            Success("Using your Client ID. Now log in below.");
            if (changed && IsLoggedIn)
                Warning("You're still logged in from the previous Client ID. Log out and log in again to use the new one.");
            RefreshAuthState();
        }

        public string StatusText()
        {
            if (!Twitch.IsInitialized)
                return "Enter your Client ID";
            return AuthStatus switch
            {
                null => "Checking login ...",
                TwitchSDK.AuthStatus.LoggedIn => "Logged in",
                TwitchSDK.AuthStatus.WaitingForCode => "Waiting for you to enter the code on Twitch",
                TwitchSDK.AuthStatus.Loading => "Loading ...",
                _ => "Logged out",
            };
        }

        /// <summary>Reads the login state (once a second, and right after anything that may change it).</summary>
        public async void RefreshAuthState()
        {
            if (!Twitch.IsInitialized || readingAuthState)
                return;
            readingAuthState = true;
            TwitchAuthState state;
            try
            {
                state = await Twitch.GetAuthState();
            }
            catch (TwitchException)
            {
                return;
            }
            finally
            {
                readingAuthState = false;
            }

            var previous = AuthStatus;
            AuthStatus = state.Status;
            GrantedScopes = state.Scopes ?? Array.Empty<string>();
            if (previous != AuthStatus)
            {
                if (AuthStatus == TwitchSDK.AuthStatus.LoggedIn)
                {
                    if (previous != null)
                        Success("Logged in to Twitch.");
                    else
                        Info("Already logged in (using the saved token).");
                }
                else if (previous == TwitchSDK.AuthStatus.LoggedIn)
                {
                    Info("Logged out.");
                }
                AuthStateChanged?.Invoke();
            }
        }

        // ------------------------------------------------------------------ log

        public void Debug(string message) => WriteLog(SampleLogLevel.Debug, message);
        public void Info(string message) => WriteLog(SampleLogLevel.Info, message);
        public void Success(string message) => WriteLog(SampleLogLevel.Success, message);
        public void Warning(string message) => WriteLog(SampleLogLevel.Warning, message);
        public void Error(string message) => WriteLog(SampleLogLevel.Error, message);

        /// <summary>Adds a line to the log panel and mirrors it to Godot's output (unless it came from there).</summary>
        public void WriteLog(SampleLogLevel level, string message, bool mirror = true)
        {
            log.Insert(0, new LogEntry { Time = Time.GetTimeStringFromSystem(), Level = level, Message = message });
            if (log.Count > MaxLogEntries)
                log.RemoveRange(MaxLogEntries, log.Count - MaxLogEntries);
            if (mirror)
            {
                switch (level)
                {
                    case SampleLogLevel.Error: GD.PushError("[TwitchSample] " + message); break;
                    case SampleLogLevel.Warning: GD.PushWarning("[TwitchSample] " + message); break;
                    default: GD.Print("[TwitchSample] " + message); break;
                }
            }
            RedrawLog();
        }

        public void ClearLog()
        {
            log.Clear();
            RedrawLog();
        }

        void RedrawLog()
        {
            if (logView == null)
                return;
            logView.Clear();
            foreach (var entry in log.Take(MaxVisibleLogEntries))
            {
                logView.PushColor(SampleUI.LevelColor(entry.Level));
                logView.AddText($"{entry.Time}   {entry.Message}");
                logView.Pop();
                logView.Newline();
            }
        }

        /// <summary>Core library messages (Twitch.LogMessage). The plugin prints them to the output already.</summary>
        void OnCoreLog(LogLevel level, string message)
        {
            if (level == LogLevel.Debug && !ShowCoreDebugMessages)
                return;
            var sampleLevel = level switch
            {
                LogLevel.Debug => SampleLogLevel.Debug,
                LogLevel.Warning => SampleLogLevel.Warning,
                LogLevel.Error => SampleLogLevel.Error,
                _ => SampleLogLevel.Info,
            };
            WriteLog(sampleLevel, "[core] " + message, false);
        }

        /// <summary>
        /// Headless smoke test (see Main.cs): shows every page and runs the input validation, without
        /// needing a login. With a window, --screenshots=&lt;folder&gt; saves a picture of every page.
        /// </summary>
        public async Task SelfTest()
        {
            var screenshots = OS.GetCmdlineUserArgs().FirstOrDefault(a => a.StartsWith("--screenshots="))?.Substring("--screenshots=".Length);
            for (var i = 0; i < tabs.GetTabCount(); i++)
            {
                tabs.CurrentTab = i;
                for (var frame = 0; frame < 5; frame++)
                    await ToSignal(GetTree(), SceneTree.SignalName.ProcessFrame);
                if (screenshots != null)
                {
                    DirAccess.MakeDirRecursiveAbsolute(screenshots);
                    GetViewport().GetTexture().GetImage().SavePng(screenshots.PathJoin($"{i}-{tabs.GetTabTitle(i)}.png"));
                }
            }
            var polls = (PollsPage)pages[1];
            polls.SelfTestStartWithoutTitle();
            var logged = log.Any(e => e.Level == SampleLogLevel.Error && e.Message.Contains("Question is required"));
            GD.Print($"SELF-TEST pages={pages.Count} logged_validation_error={logged} status={StatusText()}");
        }
    }
}
