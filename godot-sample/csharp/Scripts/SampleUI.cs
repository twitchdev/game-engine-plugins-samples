using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using Godot;

namespace TwitchSample
{
    public enum SampleLogLevel
    {
        Debug,
        Info,
        Success,
        Warning,
        Error,
    }

    /// <summary>
    /// Small helpers so every page lays out its fields and buttons the same way
    /// (SampleGUI in the Unity sample, SampleWidgets in the Unreal sample).
    /// </summary>
    public static class SampleUI
    {
        public const int LabelWidth = 200;
        public const int FieldMinWidth = 260;

        public static readonly Color Background = new Color(0.12f, 0.11f, 0.15f);
        public static readonly Color PanelColor = new Color(0.17f, 0.16f, 0.21f);
        public static readonly Color PanelBorder = new Color(0.27f, 0.25f, 0.33f);
        public static readonly Color Accent = new Color(0.75f, 0.6f, 1f);
        public static readonly Color BarColor = new Color(0.6f, 0.4f, 1f);
        public static readonly Color TextColor = new Color(0.88f, 0.88f, 0.9f);
        public static readonly Color MutedColor = new Color(0.68f, 0.68f, 0.74f);

        public static Color LevelColor(SampleLogLevel level) => level switch
        {
            SampleLogLevel.Debug => new Color(0.6f, 0.6f, 0.6f),
            SampleLogLevel.Success => new Color(0.5f, 0.9f, 0.55f),
            SampleLogLevel.Warning => new Color(1f, 0.82f, 0.35f),
            SampleLogLevel.Error => new Color(1f, 0.45f, 0.45f),
            _ => new Color(0.88f, 0.88f, 0.9f),
        };

        private static FontVariation _bold;

        // ------------------------------------------------------------------ theme

        public static Theme MakeTheme()
        {
            var theme = new Theme { DefaultFontSize = 15 };

            var panel = new StyleBoxFlat { BgColor = PanelColor, BorderColor = PanelBorder };
            panel.SetBorderWidthAll(1);
            panel.SetCornerRadiusAll(4);
            panel.SetContentMarginAll(10);
            theme.SetStylebox("panel", "PanelContainer", panel);

            // The tab bar: the selected tab in the accent color, like the Unity and Unreal samples.
            StyleBoxFlat Tab(Color color)
            {
                var style = new StyleBoxFlat { BgColor = color };
                style.SetCornerRadiusAll(4);
                style.SetContentMarginAll(8);
                return style;
            }
            theme.SetStylebox("tab_selected", "TabContainer", Tab(Accent.Darkened(0.35f)));
            theme.SetStylebox("tab_unselected", "TabContainer", Tab(new Color(0.3f, 0.3f, 0.34f)));
            theme.SetStylebox("tab_hovered", "TabContainer", Tab(new Color(0.38f, 0.36f, 0.45f)));
            theme.SetConstant("side_margin", "TabContainer", 0);
            theme.SetFont("font", "TabContainer", BoldFont());
            theme.SetStylebox("panel", "TabContainer", new StyleBoxFlat { BgColor = Background, ContentMarginTop = 8 });

            theme.SetStylebox("fill", "ProgressBar", new StyleBoxFlat { BgColor = BarColor });
            theme.SetStylebox("background", "ProgressBar", new StyleBoxFlat { BgColor = new Color(0.25f, 0.24f, 0.3f) });
            return theme;
        }

        public static Font BoldFont() => _bold ??= new FontVariation { BaseFont = ThemeDB.FallbackFont, VariationEmbolden = 0.9f };

        // ------------------------------------------------------------------ text

        public static Label MakeLabel(string text, Color? color = null, int fontSize = 0, bool bold = false, bool wrap = false)
        {
            var label = new Label { Text = text };
            label.AddThemeColorOverride("font_color", color ?? TextColor);
            if (fontSize > 0)
                label.AddThemeFontSizeOverride("font_size", fontSize);
            if (bold)
                label.AddThemeFontOverride("font", BoldFont());
            if (wrap)
            {
                label.AutowrapMode = TextServer.AutowrapMode.WordSmart;
                label.SizeFlagsHorizontal = Control.SizeFlags.ExpandFill;
            }
            return label;
        }

        public static Label Muted(string text) => MakeLabel(text, MutedColor, 0, false, true);

        public static Label LevelLabel(string text, SampleLogLevel level) => MakeLabel(text, LevelColor(level), 0, false, true);

        // ------------------------------------------------------------------ layout

        public static void AddSpacer(Control parent, int height) => parent.AddChild(new Control { CustomMinimumSize = new Vector2(0, height) });

        /// <summary>A section heading with an optional explanation underneath.</summary>
        public static void AddSection(Control parent, string heading, string description = null)
        {
            AddSpacer(parent, 10);
            parent.AddChild(MakeLabel(heading, Colors.White, 17, true));
            if (!string.IsNullOrEmpty(description))
                parent.AddChild(Muted(description));
            AddSpacer(parent, 2);
        }

        /// <summary>A collapsible section, expanded by default. Returns the container for its contents.</summary>
        public static VBoxContainer AddFoldout(Control parent, string heading, string description = null)
        {
            AddSpacer(parent, 10);
            var header = new Button { Flat = true, Alignment = HorizontalAlignment.Left, Text = "[-]  " + heading };
            header.AddThemeFontOverride("font", BoldFont());
            header.AddThemeFontSizeOverride("font_size", 17);
            header.AddThemeColorOverride("font_color", Colors.White);
            parent.AddChild(header);
            var content = new VBoxContainer();
            parent.AddChild(content);
            if (!string.IsNullOrEmpty(description))
                content.AddChild(Muted(description));
            header.Pressed += () =>
            {
                content.Visible = !content.Visible;
                header.Text = (content.Visible ? "[-]  " : "[+]  ") + heading;
            };
            return content;
        }

        /// <summary>A row with a fixed-width label on the left. Returns the row to add controls to.</summary>
        public static HBoxContainer AddRow(Control parent, string labelText)
        {
            var row = new HBoxContainer();
            var label = MakeLabel(labelText);
            // Fixed width; long labels wrap instead of pushing the field out of line.
            label.CustomMinimumSize = new Vector2(LabelWidth, 0);
            label.AutowrapMode = TextServer.AutowrapMode.WordSmart;
            row.AddChild(label);
            parent.AddChild(row);
            return row;
        }

        /// <summary>A row indented to line up with the fields. Returns the row to add buttons to.</summary>
        public static HBoxContainer AddIndented(Control parent)
        {
            var row = new HBoxContainer();
            row.AddChild(new Control { CustomMinimumSize = new Vector2(LabelWidth, 0) });
            parent.AddChild(row);
            return row;
        }

        public static Label AddHint(Control parent, string text)
        {
            var hint = Muted(text);
            AddIndented(parent).AddChild(hint);
            return hint;
        }

        public static LineEdit AddTextRow(Control parent, string labelText, string value = "", string hint = null, string placeholder = "")
        {
            var edit = new LineEdit
            {
                Text = value ?? "",
                PlaceholderText = placeholder,
                CustomMinimumSize = new Vector2(FieldMinWidth, 0),
                SizeFlagsHorizontal = Control.SizeFlags.ExpandFill,
            };
            AddRow(parent, labelText).AddChild(edit);
            if (!string.IsNullOrEmpty(hint))
                AddHint(parent, hint);
            return edit;
        }

        public static CheckBox AddCheck(Control parent, string text, bool value = false)
        {
            var box = new CheckBox { Text = text, ButtonPressed = value };
            parent.AddChild(box);
            return box;
        }

        public static OptionButton AddSelector(Control parent, string labelText, IEnumerable<string> options, int index = 0)
        {
            var option = new OptionButton();
            foreach (var o in options)
                option.AddItem(o);
            option.Select(index);
            AddRow(parent, labelText).AddChild(option);
            return option;
        }

        public static Button AddButton(Control parent, string text, Action onPressed)
        {
            var button = new Button { Text = text, CustomMinimumSize = new Vector2(0, 30) };
            button.Pressed += onPressed;
            parent.AddChild(button);
            return button;
        }

        /// <summary>A key/value line, like the result rows in the Unity and Unreal samples.</summary>
        public static Label AddKeyValue(Control parent, string key, string value)
        {
            var row = new HBoxContainer();
            var k = MakeLabel(key, MutedColor);
            k.CustomMinimumSize = new Vector2(LabelWidth, 0);
            row.AddChild(k);
            var v = MakeLabel(string.IsNullOrEmpty(value) ? "-" : value, TextColor, 0, false, true);
            row.AddChild(v);
            parent.AddChild(row);
            return v;
        }

        public static Label AddKeyValue(Control parent, string key, long value) => AddKeyValue(parent, key, Number(value));

        /// <summary>A boxed panel. Returns the container for its contents.</summary>
        public static VBoxContainer AddPanel(Control parent)
        {
            var panel = new PanelContainer();
            var box = new VBoxContainer();
            panel.AddChild(box);
            parent.AddChild(panel);
            return box;
        }

        /// <summary>A horizontal bar showing <paramref name="fraction"/> (0 to 1).</summary>
        public static ProgressBar AddBar(Control parent, float fraction)
        {
            var bar = new ProgressBar
            {
                MinValue = 0,
                MaxValue = 1,
                Value = Mathf.Clamp(fraction, 0f, 1f),
                ShowPercentage = false,
                CustomMinimumSize = new Vector2(220, 14),
                SizeFlagsVertical = Control.SizeFlags.ShrinkCenter,
            };
            parent.AddChild(bar);
            return bar;
        }

        public static void Clear(Node container)
        {
            foreach (var child in container.GetChildren())
            {
                container.RemoveChild(child);
                child.QueueFree();
            }
        }

        // ------------------------------------------------------------------ values

        /// <summary>Splits a comma, semicolon, or newline separated list and drops empty and duplicate items.</summary>
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

        public static string Number(long n) => n.ToString("N0", CultureInfo.InvariantCulture);

        public static string TierName(string tier) => tier switch
        {
            "1000" => "1",
            "2000" => "2",
            "3000" => "3",
            _ => string.IsNullOrEmpty(tier) ? "-" : tier,
        };

        /// <summary>Parses a whole number in range, logging a readable error when it isn't.</summary>
        public static bool TryParseInt(SampleApp app, string text, string fieldName, int min, int max, out int value)
        {
            if (int.TryParse(text?.Trim(), out value) && value >= min && value <= max)
                return true;
            app.Error($"{fieldName} must be a whole number from {min} to {max}.");
            return false;
        }

        /// <summary>Checks a required text field, logging a readable error when it's blank or too long.</summary>
        public static bool ValidateText(SampleApp app, string text, string fieldName, int maxLength)
        {
            if (string.IsNullOrWhiteSpace(text))
            {
                app.Error($"{fieldName} is required.");
                return false;
            }
            if (text.Trim().Length > maxLength)
            {
                app.Error($"{fieldName} can be at most {maxLength} characters.");
                return false;
            }
            return true;
        }
    }
}
