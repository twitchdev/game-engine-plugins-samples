using System;
using System.Collections.Generic;
using System.Linq;
using TwitchSDK;
using TwitchSDK.Interop;
using UnityEngine;

namespace TwitchSample
{
    /// <summary>
    /// Create a prediction from your own title and outcomes, then lock, resolve, or cancel it.
    /// </summary>
    public sealed class PredictionsPage : SamplePage
    {
        // Limits enforced by the Twitch Create Prediction endpoint.
        const int MaxTitleLength = 45;
        const int MaxOutcomeLength = 25;
        const int MinOutcomes = 2;
        const int MaxOutcomes = 10;
        const int MinDuration = 30;
        const int MaxDuration = 1800;

        string title = "Will we beat the boss?";
        readonly List<string> outcomes = new List<string> { "Yes", "No" };
        string duration = "120";

        Prediction prediction;
        PredictionInfo info; // snapshot taken in Tick

        public override string Title => "Predictions";

        public override void Tick() => info = prediction?.Info;

        public override void ReleaseApiResources()
        {
            prediction?.Dispose();
            prediction = null;
            info = null;
        }

        public override void Draw()
        {
            DrawConnectionHint();
            DrawCreateForm();
            DrawActivePrediction();
        }

        void DrawCreateForm()
        {
            SampleGUI.Section("Create a prediction",
                "Viewers bet channel points on an outcome. You need to be a Twitch Affiliate or Partner, and " +
                "Twitch allows one active prediction at a time.");

            title = SampleGUI.TextField("Title", title, $"Up to {MaxTitleLength} characters.");
            SampleGUI.EditableList("Outcome", outcomes, MinOutcomes, MaxOutcomes,
                $"{MinOutcomes} to {MaxOutcomes} outcomes, up to {MaxOutcomeLength} characters each.");
            duration = SampleGUI.TextField("Voting window (seconds)", duration,
                $"{MinDuration} to {MaxDuration}. The prediction locks when it runs out.");

            GUILayout.BeginHorizontal();
            GUILayout.Space(SampleGUI.LabelWidth + 4);
            if (SampleGUI.ActionButton(App, "Start prediction"))
                StartPrediction();
            GUILayout.EndHorizontal();
        }

        void StartPrediction()
        {
            var cleanOutcomes = outcomes.Select(x => x.Trim()).Where(x => x.Length > 0).ToArray();

            if (!SampleGUI.ValidateText(title, "Title", MaxTitleLength))
                return;
            if (cleanOutcomes.Length < MinOutcomes)
            {
                SampleLog.Error($"Enter at least {MinOutcomes} outcomes.");
                return;
            }
            var tooLong = cleanOutcomes.FirstOrDefault(x => x.Length > MaxOutcomeLength);
            if (tooLong != null)
            {
                SampleLog.Error($"Outcome \"{tooLong}\" is longer than {MaxOutcomeLength} characters.");
                return;
            }
            if (!SampleGUI.TryParseInt(duration, "Voting window", MinDuration, MaxDuration, out var seconds))
                return;

            var definition = new PredictionDefinition
            {
                Title = title.Trim(),
                Outcomes = cleanOutcomes,
                Duration = seconds,
            };

            App.Run("Start prediction", async () =>
            {
                var created = await Api.NewPrediction(definition);

                // Keep the previous prediction until the new one exists, then stop tracking it.
                prediction?.Dispose();
                prediction = created;
                info = created.Info;
                SampleLog.Success($"Prediction \"{created.Info.Title}\" started. Voting closes in {seconds} seconds.");
                WatchForLock(created);
            });
        }

        async void WatchForLock(Prediction watched)
        {
            try
            {
                await watched.PredictionLocked;
                if (watched == prediction)
                    SampleLog.Info($"Prediction \"{watched.Info.Title}\" is locked. Pick the winning outcome to pay out.");
            }
            catch (Exception e)
            {
                if (watched == prediction)
                    SampleLog.Warning("Stopped receiving prediction updates: " + TwitchSampleApp.Describe(e));
            }
        }

        void DrawActivePrediction()
        {
            SampleGUI.Section("Current prediction", "Totals update live while viewers place bets.");

            if (info == null)
            {
                GUILayout.Label("No prediction yet. Start one above.", SampleGUI.MutedStyle);
                return;
            }

            GUILayout.BeginVertical(GUI.skin.box);
            SampleGUI.KeyValue("Title", info.Title);
            SampleGUI.KeyValue("Status", info.Status.ToString());
            SampleGUI.KeyValue("Created", info.CreatedAt);
            if (!string.IsNullOrEmpty(info.LockedAt))
                SampleGUI.KeyValue("Locked", info.LockedAt);
            if (!string.IsNullOrEmpty(info.EndedAt))
                SampleGUI.KeyValue("Ended", info.EndedAt);

            bool open = info.Status == PredictionStatus.Active || info.Status == PredictionStatus.Locked;
            var predictionOutcomes = info.Outcomes ?? Array.Empty<PredictionOutcome>();
            long totalPoints = predictionOutcomes.Sum(x => x.ChannelPoints);

            GUILayout.Space(6);
            foreach (var outcome in predictionOutcomes)
            {
                bool winner = !string.IsNullOrEmpty(info.WinningOutcomeId) && outcome.Id == info.WinningOutcomeId;
                GUILayout.BeginHorizontal();
                GUILayout.Label((winner ? "Winner: " : "") + outcome.Title, GUILayout.Width(SampleGUI.LabelWidth));
                SampleGUI.Bar(totalPoints > 0 ? (float)outcome.ChannelPoints / totalPoints : 0f);
                GUILayout.Label($"  {outcome.Users:N0} viewers, {outcome.ChannelPoints:N0} points ({outcome.Color})");
                GUILayout.FlexibleSpace();
                var action = $"Resolve: {outcome.Title}";
                if (SampleGUI.ActionButton(App, action, open))
                    Resolve(action, outcome);
                GUILayout.EndHorizontal();
            }

            GUILayout.Space(6);
            GUILayout.BeginHorizontal();
            if (SampleGUI.ActionButton(App, "Lock voting now", info.Status == PredictionStatus.Active))
            {
                var target = prediction;
                App.Run("Lock voting now", async () =>
                {
                    await target.Lock();
                    SampleLog.Success("Prediction locked. Viewers can no longer place bets.");
                });
            }
            if (SampleGUI.ActionButton(App, "Cancel prediction (refund points)", open))
            {
                var target = prediction;
                App.Run("Cancel prediction (refund points)", async () =>
                {
                    await target.Cancel();
                    SampleLog.Success("Prediction canceled. Channel points were refunded.");
                });
            }
            if (SampleGUI.Button("Clear"))
                SampleGUI.Defer(ReleaseApiResources);
            GUILayout.EndHorizontal();
            GUILayout.EndVertical();
        }

        void Resolve(string action, PredictionOutcome outcome)
        {
            var target = prediction;
            App.Run(action, async () =>
            {
                await target.Resolve(outcome);
                SampleLog.Success($"Prediction resolved. \"{outcome.Title}\" won and points were paid out.");
            });
        }
    }
}
