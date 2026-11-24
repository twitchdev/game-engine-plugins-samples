using System;
using System.Collections.Generic;
using System.Linq;
using UnityEngine;

namespace TwitchSample
{
    /// <summary>
    /// Small IMGUI helpers so every page lays out its fields and buttons the same way.
    /// </summary>
    public static class SampleGUI
    {
        public const float LabelWidth = 200f;
        const float FieldMinWidth = 260f;

        static readonly Queue<Action> deferred = new Queue<Action>();
        static readonly Dictionary<string, bool> foldouts = new Dictionary<string, bool>();

        static GUIStyle background;
        static GUIStyle title;
        static GUIStyle section;
        static GUIStyle muted;
        static GUIStyle wrap;
        static GUIStyle big;
        static readonly Dictionary<SampleLogLevel, GUIStyle> logStyles = new Dictionary<SampleLogLevel, GUIStyle>();

        public static GUIStyle BackgroundStyle => background;
        public static GUIStyle TitleStyle => title;
        public static GUIStyle SectionStyle => section;
        public static GUIStyle MutedStyle => muted;
        public static GUIStyle WrapStyle => wrap;
        public static GUIStyle BigStyle => big;

        /// <summary>Creates the styles. GUI.skin is only available inside OnGUI, so call this from there.</summary>
        public static void EnsureStyles()
        {
            if (background != null)
                return;

            background = new GUIStyle();
            background.normal.background = MakeTexture(new Color(0.12f, 0.11f, 0.15f));

            title = new GUIStyle(GUI.skin.label) { fontSize = 20, fontStyle = FontStyle.Bold };
            title.normal.textColor = new Color(0.75f, 0.6f, 1f);

            section = new GUIStyle(GUI.skin.label) { fontSize = 15, fontStyle = FontStyle.Bold };
            section.normal.textColor = Color.white;

            muted = new GUIStyle(GUI.skin.label) { wordWrap = true };
            muted.normal.textColor = new Color(0.68f, 0.68f, 0.74f);

            wrap = new GUIStyle(GUI.skin.label) { wordWrap = true };

            big = new GUIStyle(GUI.skin.label) { fontSize = 28, fontStyle = FontStyle.Bold };
            big.normal.textColor = Color.white;

            logStyles[SampleLogLevel.Debug] = LogStyle(new Color(0.6f, 0.6f, 0.6f));
            logStyles[SampleLogLevel.Info] = LogStyle(new Color(0.88f, 0.88f, 0.9f));
            logStyles[SampleLogLevel.Success] = LogStyle(new Color(0.5f, 0.9f, 0.55f));
            logStyles[SampleLogLevel.Warning] = LogStyle(new Color(1f, 0.82f, 0.35f));
            logStyles[SampleLogLevel.Error] = LogStyle(new Color(1f, 0.45f, 0.45f));
        }

        static GUIStyle LogStyle(Color color)
        {
            var style = new GUIStyle(GUI.skin.label) { wordWrap = true, richText = false };
            style.normal.textColor = color;
            return style;
        }

        static Texture2D MakeTexture(Color color)
        {
            var texture = new Texture2D(1, 1) { hideFlags = HideFlags.HideAndDontSave };
            texture.SetPixel(0, 0, color);
            texture.Apply();
            return texture;
        }

        public static GUIStyle StyleFor(SampleLogLevel level) => logStyles[level];

        /// <summary>
        /// Runs <paramref name="action"/> on the next Update instead of right away.
        /// </summary>
        /// <remarks>
        /// IMGUI lays out controls in a Layout event and reuses that layout for the input event that
        /// follows. Anything a click or keystroke does that changes which controls are drawn (switching
        /// tabs, connecting, expanding a section) must wait until after OnGUI, or Unity throws
        /// "Getting control N's position in a group with only N controls".
        /// </remarks>
        public static void Defer(Action action) => deferred.Enqueue(action);

        /// <summary>Runs the actions queued with <see cref="Defer"/>. Called once per frame from Update.</summary>
        public static void RunDeferred()
        {
            while (deferred.Count > 0)
            {
                var action = deferred.Dequeue();
                try
                {
                    action();
                }
                catch (Exception e)
                {
                    SampleLog.Error("UI action failed: " + TwitchSampleApp.Describe(e));
                }
            }
        }

        /// <summary>A section heading with an optional explanation underneath.</summary>
        public static void Section(string heading, string description = null)
        {
            GUILayout.Space(14);
            GUILayout.Label(heading, section);
            if (!string.IsNullOrEmpty(description))
                GUILayout.Label(description, muted);
            GUILayout.Space(4);
        }

        /// <summary>
        /// A collapsible section heading, expanded by default. Returns whether to draw the section.
        /// The new state takes effect on the next frame (see <see cref="Defer"/>).
        /// </summary>
        public static bool Foldout(string heading, string description = null)
        {
            if (!foldouts.TryGetValue(heading, out var expanded))
                expanded = foldouts[heading] = true;

            GUILayout.Space(14);
            bool clicked = GUILayout.Toggle(expanded, (expanded ? "[-]  " : "[+]  ") + heading, section);
            if (clicked != expanded)
                Defer(() => foldouts[heading] = clicked);
            if (expanded && !string.IsNullOrEmpty(description))
                GUILayout.Label(description, muted);
            GUILayout.Space(4);
            return expanded;
        }

        public static string TextField(string label, string value, string hint = null)
        {
            GUILayout.BeginHorizontal();
            GUILayout.Label(label, GUILayout.Width(LabelWidth));
            value = GUILayout.TextField(value ?? string.Empty, GUILayout.MinWidth(FieldMinWidth));
            GUILayout.EndHorizontal();
            if (!string.IsNullOrEmpty(hint))
            {
                GUILayout.BeginHorizontal();
                GUILayout.Space(LabelWidth + 4);
                GUILayout.Label(hint, muted);
                GUILayout.EndHorizontal();
            }
            return value;
        }

        public static bool Toggle(string label, bool value) => GUILayout.Toggle(value, "  " + label);

        public static int Selector(string label, int index, string[] options)
        {
            GUILayout.BeginHorizontal();
            GUILayout.Label(label, GUILayout.Width(LabelWidth));
            index = GUILayout.Toolbar(index, options, GUILayout.ExpandWidth(false));
            GUILayout.EndHorizontal();
            return index;
        }

        public static void KeyValue(string key, string value)
        {
            GUILayout.BeginHorizontal();
            GUILayout.Label(key, muted, GUILayout.Width(LabelWidth));
            GUILayout.Label(string.IsNullOrEmpty(value) ? "-" : value, wrap);
            GUILayout.EndHorizontal();
        }

        public static void KeyValue(string key, long value) => KeyValue(key, value.ToString("N0"));

        public static bool Button(string text, bool enabled = true, float width = 0f)
        {
            var wasEnabled = GUI.enabled;
            GUI.enabled = wasEnabled && enabled;
            var clicked = width > 0f
                ? GUILayout.Button(text, GUILayout.Width(width), GUILayout.Height(26))
                : GUILayout.Button(text, GUILayout.ExpandWidth(false), GUILayout.Height(26));
            GUI.enabled = wasEnabled;
            return clicked;
        }

        /// <summary>
        /// A button for an API call. It shows "..." while the call with the same
        /// <paramref name="action"/> name is still running, and is disabled until the user is
        /// connected and, unless <paramref name="requiresLogin"/> is false, logged in.
        /// </summary>
        /// <remarks>
        /// The core library doesn't reject calls made while logged out. It holds them until a login
        /// happens, so allowing the click would leave the button waiting with no feedback.
        /// </remarks>
        public static bool ActionButton(TwitchSampleApp app, string action, bool enabled = true, bool requiresLogin = true)
        {
            var running = app.IsRunning(action);
            var ready = app.Api != null && (!requiresLogin || app.IsLoggedIn);
            return Button(running ? action + " ..." : action, enabled && ready && !running);
        }

        /// <summary>
        /// Edits a list of strings with numbered fields, plus Add and Remove buttons that keep the
        /// count between <paramref name="min"/> and <paramref name="max"/>.
        /// </summary>
        public static void EditableList(string itemLabel, List<string> items, int min, int max, string hint = null)
        {
            int removeAt = -1;
            for (int i = 0; i < items.Count; i++)
            {
                GUILayout.BeginHorizontal();
                GUILayout.Label($"{itemLabel} {i + 1}", GUILayout.Width(LabelWidth));
                items[i] = GUILayout.TextField(items[i] ?? string.Empty, GUILayout.MinWidth(FieldMinWidth));
                if (Button("Remove", items.Count > min, 80f))
                    removeAt = i;
                GUILayout.EndHorizontal();
            }

            GUILayout.BeginHorizontal();
            GUILayout.Space(LabelWidth + 4);
            if (Button($"+ Add {itemLabel.ToLowerInvariant()}", items.Count < max))
                items.Add(string.Empty);
            if (!string.IsNullOrEmpty(hint))
                GUILayout.Label(hint, muted);
            GUILayout.EndHorizontal();

            // Apply after drawing so the controls in this event match the ones laid out for it.
            if (removeAt >= 0)
                items.RemoveAt(removeAt);
        }

        /// <summary>A simple horizontal bar showing <paramref name="fraction"/> (0 to 1).</summary>
        public static void Bar(float fraction)
        {
            var rect = GUILayoutUtility.GetRect(220f, 12f, GUILayout.Width(220f));
            GUI.Box(rect, GUIContent.none);
            if (fraction > 0f)
            {
                var fill = new Rect(rect.x + 1, rect.y + 1, (rect.width - 2) * Mathf.Clamp01(fraction), rect.height - 2);
                var oldColor = GUI.color;
                GUI.color = new Color(0.6f, 0.4f, 1f);
                GUI.DrawTexture(fill, Texture2D.whiteTexture);
                GUI.color = oldColor;
            }
        }

        /// <summary>Splits a comma, semicolon, or newline separated list and drops empty items.</summary>
        public static string[] SplitList(string text)
        {
            if (string.IsNullOrWhiteSpace(text))
                return Array.Empty<string>();
            return text
                .Split(new[] { ',', ';', '\n', '\r' }, StringSplitOptions.RemoveEmptyEntries)
                .Select(x => x.Trim())
                .Where(x => x.Length > 0)
                .Distinct()
                .ToArray();
        }

        /// <summary>Returns null for blank input so optional API parameters are left unset.</summary>
        public static string NullIfBlank(string text) => string.IsNullOrWhiteSpace(text) ? null : text.Trim();

        /// <summary>Parses a whole number in range, logging a readable error when it isn't.</summary>
        public static bool TryParseInt(string text, string fieldName, int min, int max, out int value)
        {
            if (int.TryParse(text?.Trim(), out value) && value >= min && value <= max)
                return true;
            SampleLog.Error($"{fieldName} must be a whole number from {min} to {max}.");
            return false;
        }

        /// <summary>Checks a required text field, logging a readable error when it's blank or too long.</summary>
        public static bool ValidateText(string text, string fieldName, int maxLength)
        {
            if (string.IsNullOrWhiteSpace(text))
            {
                SampleLog.Error($"{fieldName} is required.");
                return false;
            }
            if (text.Trim().Length > maxLength)
            {
                SampleLog.Error($"{fieldName} can be at most {maxLength} characters.");
                return false;
            }
            return true;
        }
    }
}
