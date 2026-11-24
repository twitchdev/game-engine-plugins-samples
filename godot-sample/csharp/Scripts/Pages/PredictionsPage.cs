using System.Linq;
using Godot;
using TwitchSDK;

namespace TwitchSample
{
    /// <summary>
    /// Create a prediction from your own title and outcomes, then lock, resolve, or cancel it.
    /// </summary>
    public partial class PredictionsPage : SamplePage
    {
        // Limits enforced by the Twitch Create Prediction endpoint.
        const int MaxTitleLength = 45;
        const int MaxOutcomeLength = 25;
        const int MinOutcomes = 2;
        const int MaxOutcomes = 10;
        const int MinDuration = 30;
        const int MaxDuration = 1800;

        LineEdit titleEdit;
        StringListEditor outcomes;
        LineEdit durationEdit;
        VBoxContainer current;
        TwitchPrediction prediction;

        public override string Title => "Predictions";

        protected override void Build()
        {
            AddConnectionHint();

            SampleUI.AddSection(this, "Create a prediction",
                "Viewers bet channel points on an outcome. You need to be a Twitch Affiliate or Partner, and " +
                "Twitch allows one active prediction at a time.");
            titleEdit = SampleUI.AddTextRow(this, "Title", "Will we beat the boss?", $"Up to {MaxTitleLength} characters.");
            outcomes = new StringListEditor("Outcome", new[] { "Yes", "No" }, MinOutcomes, MaxOutcomes,
                $"{MinOutcomes} to {MaxOutcomes} outcomes, up to {MaxOutcomeLength} characters each.");
            AddChild(outcomes);
            durationEdit = SampleUI.AddTextRow(this, "Voting window (seconds)", "120",
                $"{MinDuration} to {MaxDuration}. The prediction locks when it runs out.");
            App.ActionButton(SampleUI.AddIndented(this), "Start prediction", StartPrediction);

            SampleUI.AddSection(this, "Current prediction", "Totals update live while viewers place bets.");
            current = new VBoxContainer();
            AddChild(current);
            ShowPrediction(null);
        }

        void StartPrediction()
        {
            var cleanOutcomes = outcomes.Items;
            if (!SampleUI.ValidateText(App, titleEdit.Text, "Title", MaxTitleLength))
                return;
            if (cleanOutcomes.Length < MinOutcomes)
            {
                App.Error($"Enter at least {MinOutcomes} outcomes.");
                return;
            }
            var tooLong = cleanOutcomes.FirstOrDefault(x => x.Length > MaxOutcomeLength);
            if (tooLong != null)
            {
                App.Error($"Outcome \"{tooLong}\" is longer than {MaxOutcomeLength} characters.");
                return;
            }
            if (!SampleUI.TryParseInt(App, durationEdit.Text, "Voting window", MinDuration, MaxDuration, out var seconds))
                return;

            var definition = new TwitchPredictionDefinition
            {
                Title = titleEdit.Text.Trim(),
                Outcomes = cleanOutcomes,
                Duration = seconds,
            };

            App.Run("Start prediction", async () =>
            {
                var created = await Twitch.NewPrediction(definition);
                // Keep the previous prediction until the new one exists, then stop tracking it.
                prediction?.Dispose();
                prediction = created;
                created.Updated += _ =>
                {
                    if (created == prediction)
                        ShowPrediction(created);
                };
                created.Locked += info =>
                {
                    if (created == prediction)
                        App.Info($"Prediction \"{info.Title}\" is locked. Pick the winning outcome to pay out.");
                };
                ShowPrediction(created);
                App.Success($"Prediction \"{created.Info.Title}\" started. Voting closes in {seconds} seconds.");
            });
        }

        void ShowPrediction(TwitchPrediction shown)
        {
            SampleUI.Clear(current);
            if (shown == null)
            {
                current.AddChild(SampleUI.Muted("No prediction yet. Start one above."));
                return;
            }

            var info = shown.Info;
            var box = SampleUI.AddPanel(current);
            SampleUI.AddKeyValue(box, "Title", info.Title);
            SampleUI.AddKeyValue(box, "Status", info.Status.ToString());
            SampleUI.AddKeyValue(box, "Created", info.CreatedAt);
            if (!string.IsNullOrEmpty(info.LockedAt))
                SampleUI.AddKeyValue(box, "Locked", info.LockedAt);
            if (!string.IsNullOrEmpty(info.EndedAt))
                SampleUI.AddKeyValue(box, "Ended", info.EndedAt);

            bool IsOpen()
            {
                var status = shown.Info.Status;
                return shown == prediction && (status == PredictionStatus.Active || status == PredictionStatus.Locked);
            }
            long totalPoints = info.Outcomes.Sum(x => x.ChannelPoints);

            SampleUI.AddSpacer(box, 4);
            foreach (var outcome in info.Outcomes)
            {
                var winner = !string.IsNullOrEmpty(info.WinningOutcomeId) && outcome.Id == info.WinningOutcomeId;
                var row = SampleUI.AddRow(box, (winner ? "Winner: " : "") + outcome.Title);
                SampleUI.AddBar(row, totalPoints > 0 ? (float)outcome.ChannelPoints / totalPoints : 0f);
                var details = SampleUI.MakeLabel($"  {SampleUI.Number(outcome.Users)} viewers, {SampleUI.Number(outcome.ChannelPoints)} points ({outcome.Color})");
                details.SizeFlagsHorizontal = SizeFlags.ExpandFill;
                row.AddChild(details);
                var action = "Resolve: " + outcome.Title;
                App.ActionButton(row, action, () => App.Run(action, async () =>
                {
                    await shown.Resolve(outcome);
                    App.Success($"Prediction resolved. \"{outcome.Title}\" won and points were paid out.");
                }), IsOpen);
            }

            SampleUI.AddSpacer(box, 4);
            var buttons = new HBoxContainer();
            box.AddChild(buttons);
            App.ActionButton(buttons, "Lock voting now", () => App.Run("Lock voting now", async () =>
            {
                await shown.Lock();
                App.Success("Prediction locked. Viewers can no longer place bets.");
            }), () => shown == prediction && shown.Info.Status == PredictionStatus.Active);
            App.ActionButton(buttons, "Cancel prediction (refund points)", () => App.Run("Cancel prediction (refund points)", async () =>
            {
                await shown.Cancel();
                App.Success("Prediction canceled. Channel points were refunded.");
            }), IsOpen);
            SampleUI.AddButton(buttons, "Clear", () =>
            {
                prediction?.Dispose();
                prediction = null;
                ShowPrediction(null);
            });
        }
    }
}
