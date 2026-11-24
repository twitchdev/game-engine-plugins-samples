using System;
using System.Linq;
using Godot;
using TwitchSDK;

namespace TwitchSample
{
    /// <summary>
    /// Create a poll on your channel from your own title and choices, watch votes come in, and end it.
    /// </summary>
    public partial class PollsPage : SamplePage
    {
        // Limits enforced by the Twitch Create Poll endpoint.
        const int MaxTitleLength = 60;
        const int MaxChoiceLength = 25;
        const int MinChoices = 2;
        const int MaxChoices = 5;
        const int MinDuration = 15;
        const int MaxDuration = 1800;

        LineEdit titleEdit;
        StringListEditor choices;
        LineEdit durationEdit;
        CheckBox pointsCheck;
        LineEdit pointsEdit;
        VBoxContainer current;
        TwitchPoll poll;

        public override string Title => "Polls";

        protected override void Build()
        {
            AddConnectionHint();

            SampleUI.AddSection(this, "Create a poll",
                "Runs a poll on your channel. You need to be a Twitch Affiliate or Partner, and Twitch allows " +
                "one active poll at a time.");
            titleEdit = SampleUI.AddTextRow(this, "Question", "Which path should we take?", $"Up to {MaxTitleLength} characters.");
            choices = new StringListEditor("Choice", new[] { "Left", "Right" }, MinChoices, MaxChoices,
                $"{MinChoices} to {MaxChoices} choices, up to {MaxChoiceLength} characters each.");
            AddChild(choices);
            durationEdit = SampleUI.AddTextRow(this, "Duration (seconds)", "60", $"{MinDuration} to {MaxDuration}.");
            pointsCheck = SampleUI.AddCheck(this, "Allow extra votes with channel points");
            pointsEdit = SampleUI.AddTextRow(this, "Channel points per extra vote", "100");
            pointsEdit.Editable = false;
            pointsCheck.Toggled += on => pointsEdit.Editable = on;
            App.ActionButton(SampleUI.AddIndented(this), "Start poll", StartPoll);

            SampleUI.AddSection(this, "Current poll", "Vote counts update live while the poll runs.");
            current = new VBoxContainer();
            AddChild(current);
            ShowPoll(null);
        }

        /// <summary>For the self-test: starts a poll without a question, which must log a validation error.</summary>
        public void SelfTestStartWithoutTitle()
        {
            titleEdit.Text = "";
            StartPoll();
        }

        void StartPoll()
        {
            var cleanChoices = choices.Items;
            if (!SampleUI.ValidateText(App, titleEdit.Text, "Question", MaxTitleLength))
                return;
            if (cleanChoices.Length < MinChoices)
            {
                App.Error($"Enter at least {MinChoices} choices.");
                return;
            }
            var tooLong = cleanChoices.FirstOrDefault(x => x.Length > MaxChoiceLength);
            if (tooLong != null)
            {
                App.Error($"Choice \"{tooLong}\" is longer than {MaxChoiceLength} characters.");
                return;
            }
            if (!SampleUI.TryParseInt(App, durationEdit.Text, "Duration", MinDuration, MaxDuration, out var seconds))
                return;
            var points = 0;
            if (pointsCheck.ButtonPressed && !SampleUI.TryParseInt(App, pointsEdit.Text, "Channel points per vote", 1, 1000000, out points))
                return;

            var definition = new TwitchPollDefinition
            {
                Title = titleEdit.Text.Trim(),
                Choices = cleanChoices,
                Duration = seconds,
                ChannelPointsVotingEnabled = pointsCheck.ButtonPressed,
                ChannelPointsPerVote = points,
            };

            App.Run("Start poll", async () =>
            {
                var created = await Twitch.NewPoll(definition);
                // Keep the previous poll until the new one exists, then stop tracking it.
                poll?.Dispose();
                poll = created;
                created.Updated += _ =>
                {
                    if (created == poll)
                        ShowPoll(created);
                };
                created.Ended += info =>
                {
                    if (created == poll)
                        App.Info($"Poll \"{info.Title}\" ended ({info.Status}). {Winner(info)}");
                };
                ShowPoll(created);
                App.Success($"Poll \"{created.Info.Title}\" started for {seconds} seconds.");
            });
        }

        static string Winner(TwitchPollInfo info)
        {
            var pollChoices = info.Choices ?? Array.Empty<TwitchPollChoiceInfo>();
            if (pollChoices.Length == 0 || pollChoices.All(x => x.Votes == 0))
                return "No votes were cast.";
            var top = pollChoices.OrderByDescending(x => x.Votes).First();
            return $"Most votes: \"{top.Title}\" with {SampleUI.Number(top.Votes)}.";
        }

        void ShowPoll(TwitchPoll shown)
        {
            SampleUI.Clear(current);
            if (shown == null)
            {
                current.AddChild(SampleUI.Muted("No poll yet. Start one above."));
                return;
            }

            var info = shown.Info;
            var box = SampleUI.AddPanel(current);
            SampleUI.AddKeyValue(box, "Question", info.Title);
            SampleUI.AddKeyValue(box, "Status", info.Status.ToString());
            SampleUI.AddKeyValue(box, "Started", info.StartedAt);
            if (info.Status != PollStatus.Active)
                SampleUI.AddKeyValue(box, "Ended", info.EndedAt);
            SampleUI.AddKeyValue(box, "Channel point voting",
                info.ChannelPointsVotingEnabled ? $"On, {SampleUI.Number(info.ChannelPointsPerVote)} points per vote" : "Off");

            long total = info.Choices.Sum(x => x.Votes);
            SampleUI.AddSpacer(box, 4);
            foreach (var choice in info.Choices)
            {
                var row = SampleUI.AddRow(box, choice.Title);
                SampleUI.AddBar(row, total > 0 ? (float)choice.Votes / total : 0f);
                row.AddChild(SampleUI.MakeLabel($"  {SampleUI.Number(choice.Votes)} votes ({SampleUI.Number(choice.ChannelPointsVotes)} with channel points)"));
            }
            SampleUI.AddKeyValue(box, "Total votes", total);

            SampleUI.AddSpacer(box, 4);
            var buttons = new HBoxContainer();
            box.AddChild(buttons);
            bool IsActive() => shown == poll && shown.IsActive;
            App.ActionButton(buttons, "End poll and show results", () => App.Run("End poll and show results", async () =>
            {
                await shown.FinishPoll();
                App.Success("Poll ended. Viewers can see the results.");
            }), IsActive);
            App.ActionButton(buttons, "Cancel poll (hide results)", () => App.Run("Cancel poll (hide results)", async () =>
            {
                await shown.DeletePoll();
                App.Success("Poll canceled. No results are shown to viewers.");
            }), IsActive);
            SampleUI.AddButton(buttons, "Clear", () =>
            {
                poll?.Dispose();
                poll = null;
                ShowPoll(null);
            });
        }
    }
}
