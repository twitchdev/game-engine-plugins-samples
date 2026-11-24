using Godot;
using TwitchSDK;

namespace TwitchSample
{
    /// <summary>
    /// One tab of the sample. Each page builds its controls once in <see cref="Build"/> and rebuilds
    /// its result panels when data arrives.
    /// </summary>
    public abstract partial class SamplePage : VBoxContainer
    {
        protected SampleApp App { get; private set; }
        PanelContainer hintPanel;
        Label hintLabel;

        public abstract string Title { get; }

        public void Setup(SampleApp app)
        {
            App = app;
            AddThemeConstantOverride("separation", 6);
            Build();
        }

        /// <summary>Builds the page's controls.</summary>
        protected abstract void Build();

        /// <summary>Shows a hint at the top of the page while the user still needs to set a Client ID or log in.</summary>
        protected void AddConnectionHint()
        {
            hintPanel = new PanelContainer();
            hintLabel = SampleUI.LevelLabel("", SampleLogLevel.Warning);
            hintPanel.AddChild(hintLabel);
            AddChild(hintPanel);
        }

        public override void _Process(double delta)
        {
            if (hintPanel == null || App == null)
                return;
            string hint = null;
            if (!Twitch.IsInitialized)
                hint = "No Client ID yet. Enter it on the Authorization page and press Use this Client ID.";
            else if (!App.IsLoggedIn)
                hint = "Not logged in. Log in on the Authorization page before using this page.";
            hintLabel.Text = hint ?? "";
            hintPanel.Visible = hint != null;
        }
    }
}
