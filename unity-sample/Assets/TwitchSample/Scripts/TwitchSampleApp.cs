using System;
using System.Collections.Generic;
using System.Reflection;
using System.Threading.Tasks;
using TwitchSDK;
using TwitchSDK.Interop;
using UnityEngine;

namespace TwitchSample
{
    /// <summary>
    /// Entry point of the sample: owns the API instance, the pages, and the log panel.
    /// </summary>
    /// <remarks>
    /// API results resolve on background threads. The sample awaits them from code started on the
    /// main thread, so Unity's synchronization context resumes each await on the main thread and the
    /// UI can be updated directly afterwards.
    /// </remarks>
    public sealed class TwitchSampleApp : MonoBehaviour
    {
        const float Padding = 10f;
        const float LogHeight = 190f;

        public SampleTwitchApi Api { get; private set; }

        /// <summary>The latest auth state, refreshed about once a second by the Authorization page.</summary>
        public AuthState AuthState { get; set; }

        public bool IsLoggedIn => Api != null && AuthState != null && AuthState.Status == AuthStatus.LoggedIn;

        /// <summary>The native core version, or null if the native library failed to load.</summary>
        public string CoreVersion { get; private set; }

        SamplePage[] pages;
        string[] pageTitles;
        Vector2[] pageScroll;
        int currentPage;

        readonly HashSet<string> running = new HashSet<string>();

        SampleLogEntry[] logSnapshot = Array.Empty<SampleLogEntry>();
        int logVersion = -1;
        Vector2 logScroll;

        /// <summary>
        /// Creates the sample when Play is pressed in any scene that doesn't already contain it,
        /// so the project works without opening a particular scene first.
        /// </summary>
        [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.AfterSceneLoad)]
        static void Bootstrap()
        {
            if (FindAnyObjectByType<TwitchSampleApp>() != null)
                return;

            if (Camera.main == null)
                CreateCamera();

            new GameObject("Twitch Sample").AddComponent<TwitchSampleApp>();
        }

        /// <summary>Creates a plain camera so the Game view doesn't show "No cameras rendering".</summary>
        public static Camera CreateCamera()
        {
            var go = new GameObject("Main Camera") { tag = "MainCamera" };
            var camera = go.AddComponent<Camera>();
            camera.clearFlags = CameraClearFlags.SolidColor;
            camera.backgroundColor = new Color(0.12f, 0.11f, 0.15f);
            return camera;
        }

        void Awake()
        {
            pages = new SamplePage[]
            {
                new AuthorizationPage(),
                new PollsPage(),
                new PredictionsPage(),
                new DropsPage(),
                new GeneralPage(),
            };
            pageTitles = new string[pages.Length];
            pageScroll = new Vector2[pages.Length];
            for (int i = 0; i < pages.Length; i++)
            {
                pages[i].Attach(this);
                pageTitles[i] = pages[i].Title;
            }

            try
            {
                CoreVersion = TwitchSDKApi.Version;
                SampleLog.Info($"Twitch SDK core library {CoreVersion} loaded.");
            }
            catch (Exception e)
            {
                CoreVersion = null;
                SampleLog.Error("The native Twitch SDK core library (R66_core) failed to load: " + Describe(e) +
                                ". Check that Assets/Plugins/x86_64/R66_core.dll is present (run import-plugin.bat).");
            }
        }

        /// <summary>Creates a new API instance for <paramref name="clientId"/>, replacing any existing one.</summary>
        public bool Connect(string clientId, bool useEventSubProxy)
        {
            Disconnect(log: false);
            try
            {
                Api = new SampleTwitchApi(clientId, useEventSubProxy);
                SampleLog.Success(useEventSubProxy
                    ? "Connected using the local EventSubProxy. Now log in below."
                    : "Connected. Now log in below.");
                return true;
            }
            catch (Exception e)
            {
                Api = null;
                SampleLog.Error("Could not start the Twitch SDK: " + Describe(e));
                return false;
            }
        }

        /// <summary>Disposes the API instance and everything the pages created from it.</summary>
        public void Disconnect(bool log = true)
        {
            if (Api == null)
                return;

            foreach (var page in pages)
            {
                try
                {
                    page.ReleaseApiResources();
                }
                catch (Exception e)
                {
                    SampleLog.Warning($"Cleaning up the {page.Title} page failed: {Describe(e)}");
                }
            }

            var api = Api;
            Api = null;
            AuthState = null;
            running.Clear();
            api.Dispose();

            if (log)
                SampleLog.Info("Disconnected.");
        }

        public bool IsRunning(string action) => running.Contains(action);

        /// <summary>
        /// Runs an API call: ignores repeat clicks while it's running and logs any failure with a
        /// readable message. Success messages are logged by the caller.
        /// </summary>
        /// <remarks>
        /// The call starts on the next Update rather than inside the button click, because some calls
        /// finish synchronously and would otherwise change the UI in the middle of an IMGUI event.
        /// </remarks>
        public void Run(string action, Func<Task> work)
        {
            if (Api == null)
            {
                SampleLog.Warning($"{action}: connect with your Client ID on the Authorization page first.");
                return;
            }
            if (!running.Add(action))
                return;

            SampleGUI.Defer(() => RunNow(action, work));
        }

        async void RunNow(string action, Func<Task> work)
        {
            try
            {
                if (Api == null)
                    throw new InvalidOperationException("disconnected before the call started");
                await work();
            }
            catch (Exception e)
            {
                SampleLog.Error($"{action} failed: {Describe(e)}");
            }
            finally
            {
                running.Remove(action);
            }
        }

        /// <summary>Returns the innermost meaningful exception message.</summary>
        public static string Describe(Exception e)
        {
            while ((e is AggregateException || e is TargetInvocationException) && e.InnerException != null)
                e = e.InnerException;
            return string.IsNullOrEmpty(e.Message) ? e.GetType().Name : e.Message;
        }

        void Update()
        {
            SampleGUI.RunDeferred();

            foreach (var page in pages)
            {
                try
                {
                    page.Tick();
                }
                catch (Exception e)
                {
                    SampleLog.Error($"{page.Title} page error: {Describe(e)}");
                }
            }

            SampleLog.TryUpdateSnapshot(ref logVersion, ref logSnapshot);
        }

        void OnApplicationQuit() => Disconnect(log: false);

        void OnDestroy() => Disconnect(log: false);

        void OnGUI()
        {
            SampleGUI.EnsureStyles();

            // Scale the UI up on large screens so it stays readable.
            float scale = Mathf.Clamp(Screen.height / 820f, 1f, 2.5f);
            GUI.matrix = Matrix4x4.Scale(new Vector3(scale, scale, 1f));
            float width = Screen.width / scale;
            float height = Screen.height / scale;

            GUI.Box(new Rect(0, 0, width, height), GUIContent.none, SampleGUI.BackgroundStyle);

            GUILayout.BeginArea(new Rect(Padding, Padding, width - 2 * Padding, height - 3 * Padding - LogHeight));
            DrawHeader();
            int selectedPage = GUILayout.Toolbar(currentPage, pageTitles, GUILayout.Height(32));
            if (selectedPage != currentPage)
                SampleGUI.Defer(() => currentPage = selectedPage); // switch pages between frames
            GUILayout.Space(4);
            pageScroll[currentPage] = GUILayout.BeginScrollView(pageScroll[currentPage]);
            pages[currentPage].Draw();
            GUILayout.Space(20);
            GUILayout.EndScrollView();
            GUILayout.EndArea();

            DrawLog(new Rect(Padding, height - Padding - LogHeight, width - 2 * Padding, LogHeight));
        }

        void DrawHeader()
        {
            GUILayout.BeginHorizontal();
            GUILayout.Label("Twitch SDK Unity Sample", SampleGUI.TitleStyle);
            GUILayout.FlexibleSpace();
            GUILayout.BeginVertical();
            GUILayout.Label("Core library: " + (CoreVersion ?? "failed to load"), SampleGUI.MutedStyle);
            GUILayout.Label("Status: " + StatusText(), SampleGUI.MutedStyle);
            GUILayout.EndVertical();
            GUILayout.EndHorizontal();
            GUILayout.Space(4);
        }

        string StatusText()
        {
            if (Api == null)
                return "Not connected";
            if (AuthState == null)
                return "Connected, checking login ...";
            switch (AuthState.Status)
            {
                case AuthStatus.LoggedIn: return "Logged in";
                case AuthStatus.WaitingForCode: return "Waiting for you to enter the code on Twitch";
                case AuthStatus.Loading: return "Connected, loading ...";
                default: return "Connected, logged out";
            }
        }

        void DrawLog(Rect rect)
        {
            GUILayout.BeginArea(rect, GUI.skin.box);
            GUILayout.BeginHorizontal();
            GUILayout.Label("Log (newest first)", SampleGUI.SectionStyle);
            GUILayout.FlexibleSpace();
            SampleLog.ShowCoreDebugMessages = GUILayout.Toggle(SampleLog.ShowCoreDebugMessages, "  Show core debug messages");
            GUILayout.Space(10);
            if (GUILayout.Button("Clear", GUILayout.Width(70)))
                SampleLog.Clear();
            GUILayout.EndHorizontal();

            logScroll = GUILayout.BeginScrollView(logScroll);
            foreach (var entry in logSnapshot)
                GUILayout.Label($"{entry.Time:HH:mm:ss}   {entry.Message}", SampleGUI.StyleFor(entry.Level));
            GUILayout.EndScrollView();
            GUILayout.EndArea();
        }
    }
}
