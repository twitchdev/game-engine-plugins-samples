using System;
using System.Collections.Generic;
using System.Linq;
using TwitchSDK;
using TwitchSDK.Interop;
using UnityEngine;

namespace TwitchSample
{
    /// <summary>
    /// Create a poll on your channel from your own title and choices, watch votes come in, and end it.
    /// </summary>
    public sealed class PollsPage : SamplePage
    {
        // Limits enforced by the Twitch Create Poll endpoint.
        const int MaxTitleLength = 60;
        const int MaxChoiceLength = 25;
        const int MinChoices = 2;
        const int MaxChoices = 5;
        const int MinDuration = 15;
        const int MaxDuration = 1800;

        string title = "Which path should we take?";
        readonly List<string> choices = new List<string> { "Left", "Right" };
        string duration = "60";
        bool channelPointsVoting;
        string channelPointsPerVote = "100";

        Poll poll;
        PollInfo info; // snapshot taken in Tick

        public override string Title => "Polls";

        public override void Tick() => info = poll?.Info;

        public override void ReleaseApiResources()
        {
            poll?.Dispose();
            poll = null;
            info = null;
        }

        public override void Draw()
        {
            DrawConnectionHint();
            DrawCreateForm();
            DrawActivePoll();
        }

        void DrawCreateForm()
        {
            SampleGUI.Section("Create a poll",
                "Runs a poll on your channel. You need to be a Twitch Affiliate or Partner, and Twitch allows " +
                "one active poll at a time.");

            title = SampleGUI.TextField("Question", title, $"Up to {MaxTitleLength} characters.");
            SampleGUI.EditableList("Choice", choices, MinChoices, MaxChoices,
                $"{MinChoices} to {MaxChoices} choices, up to {MaxChoiceLength} characters each.");
            duration = SampleGUI.TextField("Duration (seconds)", duration, $"{MinDuration} to {MaxDuration}.");
            channelPointsVoting = SampleGUI.Toggle("Allow extra votes with channel points", channelPointsVoting);
            // Always drawn (just disabled) so toggling doesn't add a control mid-event.
            var wasEnabled = GUI.enabled;
            GUI.enabled = wasEnabled && channelPointsVoting;
            channelPointsPerVote = SampleGUI.TextField("Channel points per extra vote", channelPointsPerVote);
            GUI.enabled = wasEnabled;

            GUILayout.BeginHorizontal();
            GUILayout.Space(SampleGUI.LabelWidth + 4);
            if (SampleGUI.ActionButton(App, "Start poll"))
                StartPoll();
            GUILayout.EndHorizontal();
        }

        void StartPoll()
        {
            var cleanChoices = choices.Select(x => x.Trim()).Where(x => x.Length > 0).ToArray();

            if (!SampleGUI.ValidateText(title, "Question", MaxTitleLength))
                return;
            if (cleanChoices.Length < MinChoices)
            {
                SampleLog.Error($"Enter at least {MinChoices} choices.");
                return;
            }
            var tooLong = cleanChoices.FirstOrDefault(x => x.Length > MaxChoiceLength);
            if (tooLong != null)
            {
                SampleLog.Error($"Choice \"{tooLong}\" is longer than {MaxChoiceLength} characters.");
                return;
            }
            if (!SampleGUI.TryParseInt(duration, "Duration", MinDuration, MaxDuration, out var seconds))
                return;
            int points = 0;
            if (channelPointsVoting && !SampleGUI.TryParseInt(channelPointsPerVote, "Channel points per vote", 1, 1000000, out points))
                return;

            var definition = new PollDefinition
            {
                Title = title.Trim(),
                Choices = cleanChoices,
                Duration = seconds,
                ChannelPointsVotingEnabled = channelPointsVoting,
                ChannelPointsPerVote = points,
            };

            App.Run("Start poll", async () =>
            {
                var created = await Api.NewPoll(definition);

                // Keep the previous poll until the new one exists, then stop tracking it.
                poll?.Dispose();
                poll = created;
                info = created.Info;
                SampleLog.Success($"Poll \"{created.Info.Title}\" started for {seconds} seconds.");
                WatchForEnd(created);
            });
        }

        async void WatchForEnd(Poll watched)
        {
            try
            {
                await watched.PollEnded;
                if (watched == poll)
                    SampleLog.Info($"Poll \"{watched.Info.Title}\" ended ({watched.Info.Status}). {Winner(watched.Info)}");
            }
            catch (Exception e)
            {
                if (watched == poll)
                    SampleLog.Warning("Stopped receiving poll updates: " + TwitchSampleApp.Describe(e));
            }
        }

        static string Winner(PollInfo pollInfo)
        {
            var choices = pollInfo.Choices ?? Array.Empty<PollChoiceInfo>();
            if (choices.Length == 0 || choices.All(x => x.Votes == 0))
                return "No votes were cast.";
            var top = choices.OrderByDescending(x => x.Votes).First();
            return $"Most votes: \"{top.Title}\" with {top.Votes:N0}.";
        }

        void DrawActivePoll()
        {
            SampleGUI.Section("Current poll", "Vote counts update live while the poll runs.");

            if (info == null)
            {
                GUILayout.Label("No poll yet. Start one above.", SampleGUI.MutedStyle);
                return;
            }

            GUILayout.BeginVertical(GUI.skin.box);
            SampleGUI.KeyValue("Question", info.Title);
            SampleGUI.KeyValue("Status", info.Status.ToString());
            SampleGUI.KeyValue("Started", info.StartedAt);
            if (info.Status != PollStatus.Active)
                SampleGUI.KeyValue("Ended", info.EndedAt);
            SampleGUI.KeyValue("Channel point voting", info.ChannelPointsVotingEnabled ? $"On, {info.ChannelPointsPerVote:N0} points per vote" : "Off");

            var pollChoices = info.Choices ?? Array.Empty<PollChoiceInfo>();
            long total = pollChoices.Sum(x => x.Votes);
            GUILayout.Space(6);
            foreach (var choice in pollChoices)
            {
                GUILayout.BeginHorizontal();
                GUILayout.Label(choice.Title, GUILayout.Width(SampleGUI.LabelWidth));
                SampleGUI.Bar(total > 0 ? (float)choice.Votes / total : 0f);
                GUILayout.Label($"  {choice.Votes:N0} votes ({choice.ChannelPointsVotes:N0} with channel points)");
                GUILayout.EndHorizontal();
            }
            SampleGUI.KeyValue("Total votes", total);

            bool active = info.Status == PollStatus.Active;
            GUILayout.Space(6);
            GUILayout.BeginHorizontal();
            if (SampleGUI.ActionButton(App, "End poll and show results", active))
            {
                var target = poll;
                App.Run("End poll and show results", async () =>
                {
                    await target.FinishPoll();
                    SampleLog.Success("Poll ended. Viewers can see the results.");
                });
            }
            if (SampleGUI.ActionButton(App, "Cancel poll (hide results)", active))
            {
                var target = poll;
                App.Run("Cancel poll (hide results)", async () =>
                {
                    await target.DeletePoll();
                    SampleLog.Success("Poll canceled. No results are shown to viewers.");
                });
            }
            if (SampleGUI.Button("Clear"))
                SampleGUI.Defer(ReleaseApiResources);
            GUILayout.EndHorizontal();
            GUILayout.EndVertical();
        }
    }
}
