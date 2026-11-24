using System.Collections.Generic;
using System.Linq;
using Godot;

namespace TwitchSample
{
    /// <summary>
    /// Edits a list of strings with numbered fields plus Add and Remove buttons that keep the count
    /// between a minimum and a maximum (poll choices, prediction outcomes).
    /// </summary>
    public partial class StringListEditor : VBoxContainer
    {
        readonly string itemLabel;
        readonly int min;
        readonly int max;
        readonly VBoxContainer rows = new VBoxContainer();
        readonly List<(HBoxContainer Row, LineEdit Edit, Button Remove)> items = new List<(HBoxContainer, LineEdit, Button)>();
        readonly Button addButton;

        public StringListEditor() { }

        public StringListEditor(string itemLabel, IEnumerable<string> initial, int min, int max, string hint = null)
        {
            this.itemLabel = itemLabel;
            this.min = min;
            this.max = max;
            AddChild(rows);
            var footer = SampleUI.AddIndented(this);
            addButton = SampleUI.AddButton(footer, "+ Add " + itemLabel.ToLowerInvariant(), () => AddItem(""));
            if (!string.IsNullOrEmpty(hint))
                footer.AddChild(SampleUI.Muted(hint));
            foreach (var item in initial)
                AddItem(item);
            UpdateRows();
        }

        /// <summary>The trimmed, non-empty items.</summary>
        public string[] Items => items.Select(x => x.Edit.Text.Trim()).Where(x => x.Length > 0).ToArray();

        void AddItem(string text)
        {
            if (items.Count >= max)
                return;
            var row = SampleUI.AddRow(rows, "");
            var edit = new LineEdit
            {
                Text = text,
                CustomMinimumSize = new Vector2(SampleUI.FieldMinWidth, 0),
                SizeFlagsHorizontal = SizeFlags.ExpandFill,
            };
            row.AddChild(edit);
            Button remove = null;
            remove = SampleUI.AddButton(row, "Remove", () =>
            {
                if (items.Count <= min)
                    return;
                items.RemoveAll(x => x.Row == row);
                rows.RemoveChild(row);
                row.QueueFree();
                UpdateRows();
            });
            remove.CustomMinimumSize = new Vector2(80, 30);
            items.Add((row, edit, remove));
            UpdateRows();
        }

        void UpdateRows()
        {
            for (var i = 0; i < items.Count; i++)
            {
                ((Label)items[i].Row.GetChild(0)).Text = $"{itemLabel} {i + 1}";
                items[i].Remove.Disabled = items.Count <= min;
            }
            if (addButton != null)
                addButton.Disabled = items.Count >= max;
        }
    }
}
