using UnityEngine;

namespace TwitchSample
{
    /// <summary>
    /// One tab of the sample. Pages are plain classes owned by <see cref="TwitchSampleApp"/>.
    /// </summary>
    public abstract class SamplePage
    {
        protected TwitchSampleApp App { get; private set; }

        /// <summary>The current API instance, or null while not connected.</summary>
        protected SampleTwitchApi Api => App.Api;

        public abstract string Title { get; }

        internal void Attach(TwitchSampleApp app) => App = app;

        /// <summary>
        /// Runs every frame from Update on the main thread. Copy any state that background work
        /// changes here, so OnGUI draws the same thing in its Layout and Repaint passes.
        /// </summary>
        public virtual void Tick()
        {
        }

        /// <summary>Draws the page. Runs from OnGUI.</summary>
        public abstract void Draw();

        /// <summary>
        /// Called before the API instance is replaced or disposed. Dispose of anything created
        /// from it (polls, predictions, event streams) and clear cached results.
        /// </summary>
        public virtual void ReleaseApiResources()
        {
        }

        /// <summary>Shows a hint at the top of the page when the user still needs to connect or log in.</summary>
        protected void DrawConnectionHint()
        {
            string hint = null;
            if (Api == null)
                hint = "Not connected. Enter your Client ID on the Authorization page and press Connect.";
            else if (!App.IsLoggedIn)
                hint = "Connected but not logged in. Log in on the Authorization page before using this page.";

            if (hint != null)
            {
                GUILayout.BeginVertical(GUI.skin.box);
                GUILayout.Label(hint, SampleGUI.StyleFor(SampleLogLevel.Warning));
                GUILayout.EndVertical();
            }
        }
    }
}
