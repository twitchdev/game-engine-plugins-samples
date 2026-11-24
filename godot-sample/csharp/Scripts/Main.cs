using System.Linq;
using Godot;

namespace TwitchSample
{
    /// <summary>
    /// Entry point. Checks that the TwitchSDK addon's native library is loaded before starting the sample.
    /// </summary>
    public partial class Main : Control
    {
        public override async void _Ready()
        {
            if (!Engine.HasSingleton("Twitch"))
            {
                var message = new Label
                {
                    Text = "The TwitchSDK addon isn't installed.\n\nRun import-plugin.bat in the godot-sample folder (after building " +
                           "the addon with TwitchSDK-Godot\\build.bat), then reopen the project.",
                    HorizontalAlignment = HorizontalAlignment.Center,
                    VerticalAlignment = VerticalAlignment.Center,
                    AutowrapMode = TextServer.AutowrapMode.WordSmart,
                };
                message.SetAnchorsPreset(LayoutPreset.FullRect);
                AddChild(message);
                GD.PushError("TwitchSDK addon not found (addons/twitchsdk). Run import-plugin.bat.");
                return;
            }

            var app = new SampleApp();
            app.SetAnchorsPreset(LayoutPreset.FullRect);
            AddChild(app);

            // Headless smoke test: godot --headless --path . -- --self-test
            if (OS.GetCmdlineUserArgs().Contains("--self-test"))
            {
                await app.SelfTest();
                GetTree().Quit();
            }
        }
    }
}
