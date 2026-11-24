using System;
using System.Collections.Generic;
using System.Linq;
using TwitchSDK;
using TwitchSDK.Interop;
using UnityEngine;

namespace TwitchSample
{
    /// <summary>
    /// Everything that doesn't belong to another page: users, streams, subscriptions, channel edits,
    /// stream markers, clips, the Bits leaderboard, channel point rewards, and live events.
    /// </summary>
    public sealed class GeneralPage : SamplePage
    {
        const int MaxFeedEntries = 50;
        static readonly string[] LeaderboardPeriods = { "day", "week", "month", "year", "all" };

        // Users
        string lookupLogin = string.Empty;
        string lookupId = string.Empty;
        UserInfo user;

        // Streams
        StreamInfo myStream;
        bool myStreamLoaded;
        string streamLogins = string.Empty;
        StreamInfo[] foundStreams;

        // Subscription check
        string subscriptionBroadcasterId = string.Empty;
        UserSubscriptionCheckResult subscription;
        string subscriptionCheckedFor;

        // Channel information
        string channelTitle = string.Empty;
        string channelGameId = string.Empty;
        string channelLanguage = string.Empty;
        string channelTags = string.Empty;
        bool clearTags;

        // Markers and clips
        string markerDescription = "Sample marker";
        StreamMarkerInfo marker;
        bool clipHasDelay;
        ClipInfo clip;

        // Bits leaderboard
        string leaderboardCount = "10";
        int leaderboardPeriod = 4;
        BitsLeaderboard leaderboard;

        // Channel point rewards
        string rewardTitle = "Sample reward";
        string rewardCost = "100";
        string rewardPrompt = "Redeemed from the Twitch SDK Unity sample";
        bool rewardNeedsInput;
        readonly List<CustomRewardEvent> pendingRedemptions = new List<CustomRewardEvent>();

        // Live events
        readonly EventSubscription<ChannelFollowEvent> follows;
        readonly EventSubscription<ChannelSubscribeEvent> subscriptions;
        readonly EventSubscription<ChannelCheerEvent> cheers;
        readonly EventSubscription<ChannelRaidEvent> raids;
        readonly EventSubscription<HypeTrainEvent> hypeTrains;
        readonly EventSubscription<CustomRewardEvent> redemptions;
        readonly List<string> feed = new List<string>();

        public override string Title => "General";

        public GeneralPage()
        {
            follows = new EventSubscription<ChannelFollowEvent>("follows",
                api => api.SubscribeToChannelFollowEvents(),
                e => AddToFeed($"{e.UserDisplayName} followed the channel."));

            subscriptions = new EventSubscription<ChannelSubscribeEvent>("subscriptions",
                api => api.SubscribeToChannelSubscribeEvents(),
                e => AddToFeed(e.IsGift
                    ? $"{e.UserDisplayName} received a gifted tier {TierName(e.Tier)} subscription."
                    : $"{e.UserDisplayName} subscribed at tier {TierName(e.Tier)} ({e.CumulativeMonths} months total)."));

            cheers = new EventSubscription<ChannelCheerEvent>("cheers",
                api => api.SubscribeToChannelCheerEvents(),
                e => AddToFeed($"{(e.IsAnonymous ? "An anonymous viewer" : e.UserDisplayName)} cheered {e.Bits:N0} bits: {e.Message}"));

            raids = new EventSubscription<ChannelRaidEvent>("raids",
                api => api.SubscribeToChannelRaidEvents(),
                e => AddToFeed($"{e.FromBroadcasterName} raided {e.ToBroadcasterName} with {e.Viewers:N0} viewers."));

            hypeTrains = new EventSubscription<HypeTrainEvent>("Hype Train updates",
                api => api.SubscribeToHypeTrainEvents(),
                e => AddToFeed(string.IsNullOrEmpty(e.EndedAt)
                    ? $"Hype Train level {e.Level}: {e.Progress:N0} / {e.Goal:N0} points."
                    : $"Hype Train ended at level {e.Level} with {e.TotalPoints:N0} points."));

            redemptions = new EventSubscription<CustomRewardEvent>("channel point redemptions",
                api => api.SubscribeToCustomRewardEvents(),
                OnRedemption);
        }

        /// <summary>Stops live events after a log out so the toggles match what's actually running.</summary>
        public override void Tick()
        {
            if (!App.IsLoggedIn && App.AuthState != null)
                StopAllEvents(log: true);
        }

        void StopAllEvents(bool log)
        {
            follows.Stop(log);
            subscriptions.Stop(log);
            cheers.Stop(log);
            raids.Stop(log);
            hypeTrains.Stop(log);
            redemptions.Stop(log);
        }

        public override void ReleaseApiResources()
        {
            StopAllEvents(log: false);

            user = null;
            myStream = null;
            myStreamLoaded = false;
            foundStreams = null;
            subscription = null;
            marker = null;
            clip = null;
            leaderboard = null;
            pendingRedemptions.Clear();
        }

        public override void Draw()
        {
            DrawConnectionHint();
            DrawUsers();
            DrawStreams();
            DrawSubscriptionCheck();
            DrawChannelInfo();
            DrawMarkersAndClips();
            DrawBitsLeaderboard();
            DrawRewards();
            DrawEvents();
        }

        // ------------------------------------------------------------------ Users

        void DrawUsers()
        {
            if (!SampleGUI.Foldout("Users", "Look up any Twitch user by login name or user ID."))
                return;

            GUILayout.BeginHorizontal();
            GUILayout.Space(SampleGUI.LabelWidth + 4);
            if (SampleGUI.ActionButton(App, "Get my user info"))
                App.Run("Get my user info", async () => ShowUser(await Api.GetMyUserInfo()));
            GUILayout.EndHorizontal();

            lookupLogin = SampleGUI.TextField("Login name", lookupLogin);
            GUILayout.BeginHorizontal();
            GUILayout.Space(SampleGUI.LabelWidth + 4);
            if (SampleGUI.ActionButton(App, "Look up by login", !string.IsNullOrWhiteSpace(lookupLogin)))
            {
                var login = lookupLogin.Trim();
                App.Run("Look up by login", async () => ShowUser(await Api.GetUserInfoByLoginName(login)));
            }
            GUILayout.EndHorizontal();

            lookupId = SampleGUI.TextField("User ID", lookupId);
            GUILayout.BeginHorizontal();
            GUILayout.Space(SampleGUI.LabelWidth + 4);
            if (SampleGUI.ActionButton(App, "Look up by ID", !string.IsNullOrWhiteSpace(lookupId)))
            {
                var id = lookupId.Trim();
                App.Run("Look up by ID", async () => ShowUser(await Api.GetUserInfoById(id)));
            }
            GUILayout.EndHorizontal();

            if (user == null)
                return;

            GUILayout.BeginVertical(GUI.skin.box);
            SampleGUI.KeyValue("Display name", user.DisplayName);
            SampleGUI.KeyValue("Login", user.LoginName);
            SampleGUI.KeyValue("User ID", user.ChannelId);
            SampleGUI.KeyValue("Broadcaster type", string.IsNullOrEmpty(user.BroadcasterType) ? "(none)" : user.BroadcasterType);
            SampleGUI.KeyValue("Description", user.Description);
            SampleGUI.KeyValue("Created", user.CreatedAt);
            SampleGUI.KeyValue("Profile image", user.ProfileImageUrl);
            GUILayout.BeginHorizontal();
            GUILayout.Space(SampleGUI.LabelWidth + 4);
            if (SampleGUI.Button("Use this ID for the subscription check"))
                subscriptionBroadcasterId = user.ChannelId;
            if (SampleGUI.Button("Open channel") && !string.IsNullOrEmpty(user.LoginName))
                Application.OpenURL("https://www.twitch.tv/" + user.LoginName);
            GUILayout.EndHorizontal();
            GUILayout.EndVertical();
        }

        void ShowUser(UserInfo info)
        {
            user = info;
            if (info == null || string.IsNullOrEmpty(info.ChannelId))
                SampleLog.Warning("No user found.");
            else
                SampleLog.Success($"Found {info.DisplayName} (user ID {info.ChannelId}).");
        }

        // ------------------------------------------------------------------ Streams

        void DrawStreams()
        {
            if (!SampleGUI.Foldout("Streams", "See your own stream and find out which channels are live."))
                return;

            GUILayout.BeginHorizontal();
            GUILayout.Space(SampleGUI.LabelWidth + 4);
            if (SampleGUI.ActionButton(App, "Get my stream info"))
            {
                App.Run("Get my stream info", async () =>
                {
                    myStream = await Api.GetMyStreamInfo();
                    myStreamLoaded = true;
                    SampleLog.Success(IsLive(myStream) ? $"You are live: {myStream.Title}" : "Your channel is offline.");
                });
            }
            GUILayout.EndHorizontal();

            if (myStreamLoaded)
            {
                GUILayout.BeginVertical(GUI.skin.box);
                if (IsLive(myStream))
                    DrawStream(myStream);
                else
                    GUILayout.Label("Your channel is offline.", SampleGUI.MutedStyle);
                GUILayout.EndVertical();
            }

            streamLogins = SampleGUI.TextField("Channel logins", streamLogins, "Separate several logins with commas (up to 100).");
            GUILayout.BeginHorizontal();
            GUILayout.Space(SampleGUI.LabelWidth + 4);
            var logins = SampleGUI.SplitList(streamLogins);
            if (SampleGUI.ActionButton(App, "Find live streams", logins.Length > 0))
            {
                App.Run("Find live streams", async () =>
                {
                    var result = await Api.QueryStreams(new StreamQuery { UserLogins = logins, First = Math.Min(logins.Length, 100) });
                    foundStreams = result.Streams ?? Array.Empty<StreamInfo>();
                    SampleLog.Success($"{foundStreams.Length} of {logins.Length} channel(s) are live.");
                });
            }
            GUILayout.EndHorizontal();

            if (foundStreams != null)
            {
                if (foundStreams.Length == 0)
                    GUILayout.Label("None of those channels are live.", SampleGUI.MutedStyle);
                foreach (var stream in foundStreams)
                {
                    GUILayout.BeginVertical(GUI.skin.box);
                    DrawStream(stream);
                    GUILayout.EndVertical();
                }
            }
        }

        static bool IsLive(StreamInfo stream) => stream != null && !string.IsNullOrEmpty(stream.Id);

        static void DrawStream(StreamInfo stream)
        {
            SampleGUI.KeyValue("Channel", stream.UserName);
            SampleGUI.KeyValue("Title", stream.Title);
            SampleGUI.KeyValue("Category", stream.GameName);
            SampleGUI.KeyValue("Viewers", stream.ViewerCount);
            SampleGUI.KeyValue("Started", stream.StartedAt);
            SampleGUI.KeyValue("Language", stream.Language);
            if (stream.Tags != null && stream.Tags.Length > 0)
                SampleGUI.KeyValue("Tags", string.Join(", ", stream.Tags));
        }

        // ------------------------------------------------------------------ Subscription check

        void DrawSubscriptionCheck()
        {
            if (!SampleGUI.Foldout("Subscription check",
                    "Checks whether the logged-in user subscribes to a channel. Find a channel's user ID with the Users section above."))
                return;

            subscriptionBroadcasterId = SampleGUI.TextField("Broadcaster user ID", subscriptionBroadcasterId);
            GUILayout.BeginHorizontal();
            GUILayout.Space(SampleGUI.LabelWidth + 4);
            if (SampleGUI.ActionButton(App, "Check my subscription", !string.IsNullOrWhiteSpace(subscriptionBroadcasterId)))
            {
                var id = subscriptionBroadcasterId.Trim();
                App.Run("Check my subscription", async () =>
                {
                    subscription = await Api.CheckUserSubscription(id);
                    subscriptionCheckedFor = id;
                    SampleLog.Success(subscription.IsSubscribed
                        ? $"You subscribe to {id} at tier {TierName(subscription.Tier)}."
                        : $"You don't subscribe to {id}.");
                });
            }
            GUILayout.EndHorizontal();

            if (subscription == null)
                return;

            GUILayout.BeginVertical(GUI.skin.box);
            SampleGUI.KeyValue("Broadcaster ID", subscriptionCheckedFor);
            SampleGUI.KeyValue("Subscribed", subscription.IsSubscribed ? "Yes" : "No");
            if (subscription.IsSubscribed)
            {
                SampleGUI.KeyValue("Tier", TierName(subscription.Tier));
                SampleGUI.KeyValue("Gift", subscription.IsGift ? $"Yes, from {subscription.GifterName}" : "No");
            }
            GUILayout.EndVertical();
        }

        static string TierName(string tier)
        {
            switch (tier)
            {
                case "1000": return "1";
                case "2000": return "2";
                case "3000": return "3";
                default: return string.IsNullOrEmpty(tier) ? "-" : tier;
            }
        }

        // ------------------------------------------------------------------ Channel information

        void DrawChannelInfo()
        {
            if (!SampleGUI.Foldout("Channel information",
                    "Updates your channel. Fill in only the fields you want to change."))
                return;

            channelTitle = SampleGUI.TextField("Stream title", channelTitle);
            channelGameId = SampleGUI.TextField("Category (game) ID", channelGameId, "A numeric category ID, e.g. 509658 for Just Chatting.");
            channelLanguage = SampleGUI.TextField("Language", channelLanguage, "A two-letter code such as en or de.");
            channelTags = SampleGUI.TextField("Tags", channelTags, "Separate tags with commas. Tags can't contain spaces.");
            clearTags = SampleGUI.Toggle("Remove all tags", clearTags);

            GUILayout.BeginHorizontal();
            GUILayout.Space(SampleGUI.LabelWidth + 4);
            if (SampleGUI.ActionButton(App, "Load current values"))
            {
                App.Run("Load current values", async () =>
                {
                    var stream = await Api.GetMyStreamInfo();
                    if (!IsLive(stream))
                    {
                        SampleLog.Info("Your channel is offline, so there are no current values to load. Enter them manually.");
                        return;
                    }
                    channelTitle = stream.Title ?? string.Empty;
                    channelGameId = stream.GameId ?? string.Empty;
                    channelLanguage = stream.Language ?? string.Empty;
                    channelTags = stream.Tags == null ? string.Empty : string.Join(", ", stream.Tags);
                    SampleLog.Success("Loaded the current channel values.");
                });
            }
            if (SampleGUI.ActionButton(App, "Update channel"))
                UpdateChannel();
            GUILayout.EndHorizontal();
        }

        void UpdateChannel()
        {
            var title = SampleGUI.NullIfBlank(channelTitle);
            var gameId = SampleGUI.NullIfBlank(channelGameId);
            var language = SampleGUI.NullIfBlank(channelLanguage);
            var tagList = SampleGUI.SplitList(channelTags);
            string[] tags = clearTags ? Array.Empty<string>() : (tagList.Length > 0 ? tagList : null);

            if (title == null && gameId == null && language == null && tags == null)
            {
                SampleLog.Error("Fill in at least one field to change.");
                return;
            }

            App.Run("Update channel", async () =>
            {
                await Api.ModifyChannelInformation(gameId: gameId, language: language, title: title, tags: tags);
                SampleLog.Success("Channel information updated.");
            });
        }

        // ------------------------------------------------------------------ Markers and clips

        void DrawMarkersAndClips()
        {
            if (!SampleGUI.Foldout("Stream markers and clips", "Both need your channel to be live."))
                return;

            markerDescription = SampleGUI.TextField("Marker description", markerDescription, "Up to 140 characters.");
            GUILayout.BeginHorizontal();
            GUILayout.Space(SampleGUI.LabelWidth + 4);
            if (SampleGUI.ActionButton(App, "Create stream marker"))
            {
                var description = markerDescription.Trim();
                App.Run("Create stream marker", async () =>
                {
                    marker = await Api.CreateStreamMarker(description);
                    SampleLog.Success($"Marker created {marker.PositionSeconds} seconds into the stream.");
                });
            }
            GUILayout.EndHorizontal();

            if (marker != null)
            {
                GUILayout.BeginVertical(GUI.skin.box);
                SampleGUI.KeyValue("Marker ID", marker.Id);
                SampleGUI.KeyValue("Description", marker.Description);
                SampleGUI.KeyValue("Position", marker.PositionSeconds + " seconds");
                SampleGUI.KeyValue("Created", marker.CreatedAt);
                GUILayout.EndVertical();
            }

            GUILayout.Space(8);
            clipHasDelay = SampleGUI.Toggle("Account for the stream delay when clipping", clipHasDelay);
            GUILayout.BeginHorizontal();
            GUILayout.Space(SampleGUI.LabelWidth + 4);
            if (SampleGUI.ActionButton(App, "Create clip"))
            {
                var delay = clipHasDelay;
                App.Run("Create clip", async () =>
                {
                    SampleLog.Info("Creating a clip. Twitch takes a few seconds to process it ...");
                    clip = await Api.CreateClip(delay);
                    SampleLog.Success("Clip created: " + clip.Url);
                });
            }
            GUILayout.EndHorizontal();

            if (clip != null)
            {
                GUILayout.BeginVertical(GUI.skin.box);
                SampleGUI.KeyValue("Clip ID", clip.Id);
                SampleGUI.KeyValue("URL", clip.Url);
                SampleGUI.KeyValue("Duration", clip.Duration + " seconds");
                GUILayout.BeginHorizontal();
                GUILayout.Space(SampleGUI.LabelWidth + 4);
                if (SampleGUI.Button("Open clip", !string.IsNullOrEmpty(clip.Url)))
                    Application.OpenURL(clip.Url);
                GUILayout.EndHorizontal();
                GUILayout.EndVertical();
            }
        }

        // ------------------------------------------------------------------ Bits leaderboard

        void DrawBitsLeaderboard()
        {
            if (!SampleGUI.Foldout("Bits leaderboard", "The top cheerers on your channel."))
                return;

            leaderboardCount = SampleGUI.TextField("Entries", leaderboardCount, "1 to 100.");
            leaderboardPeriod = SampleGUI.Selector("Period", leaderboardPeriod, LeaderboardPeriods);

            GUILayout.BeginHorizontal();
            GUILayout.Space(SampleGUI.LabelWidth + 4);
            if (SampleGUI.ActionButton(App, "Get Bits leaderboard") &&
                SampleGUI.TryParseInt(leaderboardCount, "Entries", 1, 100, out var count))
            {
                var period = LeaderboardPeriods[leaderboardPeriod];
                App.Run("Get Bits leaderboard", async () =>
                {
                    leaderboard = await Api.GetBitsLeaderboard(count: count, period: period);
                    SampleLog.Success($"Loaded {leaderboard.Data?.Length ?? 0} leaderboard entries.");
                });
            }
            GUILayout.EndHorizontal();

            if (leaderboard == null)
                return;

            GUILayout.BeginVertical(GUI.skin.box);
            var entries = leaderboard.Data ?? Array.Empty<BitsLeaderboardEntry>();
            if (entries.Length == 0)
                GUILayout.Label("Nobody has cheered in this period.", SampleGUI.MutedStyle);
            foreach (var entry in entries)
                SampleGUI.KeyValue($"#{entry.Rank}  {entry.UserName}", $"{entry.Score:N0} bits");
            GUILayout.EndVertical();
        }

        // ------------------------------------------------------------------ Channel point rewards

        void DrawRewards()
        {
            if (!SampleGUI.Foldout("Channel point rewards",
                    "Creates a custom reward on your channel. Replacing rewards affects only rewards made with this " +
                    "Client ID. Turn on channel point redemptions under Live events to see and resolve redemptions."))
                return;

            rewardTitle = SampleGUI.TextField("Reward title", rewardTitle, "Up to 45 characters, unique on your channel.");
            rewardCost = SampleGUI.TextField("Cost (channel points)", rewardCost);
            rewardPrompt = SampleGUI.TextField("Prompt", rewardPrompt);
            rewardNeedsInput = SampleGUI.Toggle("Viewers must enter text when redeeming", rewardNeedsInput);

            GUILayout.BeginHorizontal();
            GUILayout.Space(SampleGUI.LabelWidth + 4);
            if (SampleGUI.ActionButton(App, "Create reward") &&
                SampleGUI.ValidateText(rewardTitle, "Reward title", 45) &&
                SampleGUI.TryParseInt(rewardCost, "Cost", 1, int.MaxValue, out var cost))
            {
                var definition = new CustomRewardDefinition
                {
                    Title = rewardTitle.Trim(),
                    Cost = cost,
                    Prompt = rewardPrompt?.Trim() ?? string.Empty,
                    IsEnabled = true,
                    IsUserInputRequired = rewardNeedsInput,
                };
                App.Run("Create reward", async () =>
                {
                    await Api.ReplaceCustomRewards(definition);
                    SampleLog.Success($"Reward \"{definition.Title}\" is now on your channel for {cost:N0} points.");
                });
            }
            if (SampleGUI.ActionButton(App, "Remove sample rewards"))
            {
                App.Run("Remove sample rewards", async () =>
                {
                    await Api.ReplaceCustomRewards();
                    SampleLog.Success("Removed the rewards created with this Client ID.");
                });
            }
            GUILayout.EndHorizontal();

            if (pendingRedemptions.Count == 0)
                return;

            GUILayout.Space(6);
            GUILayout.Label("Waiting redemptions", SampleGUI.SectionStyle);
            CustomRewardEvent resolved = null;
            foreach (var redemption in pendingRedemptions)
            {
                GUILayout.BeginVertical(GUI.skin.box);
                SampleGUI.KeyValue("Reward", redemption.CustomRewardTitle);
                SampleGUI.KeyValue("Viewer", redemption.RedeemerName);
                if (!string.IsNullOrEmpty(redemption.UserInput))
                    SampleGUI.KeyValue("Their text", redemption.UserInput);
                SampleGUI.KeyValue("Redeemed", redemption.RedeemedAt);

                GUILayout.BeginHorizontal();
                GUILayout.Space(SampleGUI.LabelWidth + 4);
                var fulfill = "Fulfill " + redemption.RedemptionId;
                var refund = "Refund " + redemption.RedemptionId;
                if (SampleGUI.ActionButton(App, fulfill))
                    Resolve(fulfill, redemption, CustomRewardRedemptionState.Fulfilled);
                if (SampleGUI.ActionButton(App, refund))
                    Resolve(refund, redemption, CustomRewardRedemptionState.Canceled);
                if (SampleGUI.Button("Dismiss"))
                    resolved = redemption;
                GUILayout.EndHorizontal();
                GUILayout.EndVertical();
            }
            if (resolved != null)
                pendingRedemptions.Remove(resolved);
        }

        void Resolve(string action, CustomRewardEvent redemption, CustomRewardRedemptionState state)
        {
            App.Run(action, async () =>
            {
                await Api.ResolveCustomReward(redemption, state);
                pendingRedemptions.Remove(redemption);
                SampleLog.Success(state == CustomRewardRedemptionState.Fulfilled
                    ? $"Fulfilled {redemption.RedeemerName}'s \"{redemption.CustomRewardTitle}\"."
                    : $"Refunded {redemption.RedeemerName}'s \"{redemption.CustomRewardTitle}\".");
            });
        }

        void OnRedemption(CustomRewardEvent e)
        {
            AddToFeed($"{e.RedeemerName} redeemed \"{e.CustomRewardTitle}\" for {e.CustomRewardCost:N0} points." +
                      (string.IsNullOrEmpty(e.UserInput) ? "" : $" They wrote: {e.UserInput}"));

            // Only redemptions still in the queue can be fulfilled or refunded.
            if (e.Status == CustomRewardRedemptionState.Unfulfilled &&
                pendingRedemptions.All(x => x.RedemptionId != e.RedemptionId))
                pendingRedemptions.Add(e);
        }

        // ------------------------------------------------------------------ Live events

        void DrawEvents()
        {
            if (!SampleGUI.Foldout("Live events (EventSub)",
                    "Turn on the events you want to see. To test without real viewers, enable the local " +
                    "EventSubProxy on the Authorization page and send test events with the Twitch CLI."))
                return;

            DrawSubscriptionToggle(follows, "Follows (moderator:read:followers)");
            DrawSubscriptionToggle(subscriptions, "Subscriptions (channel:read:subscriptions)");
            DrawSubscriptionToggle(cheers, "Cheers (bits:read)");
            DrawSubscriptionToggle(raids, "Raids (no scope needed)");
            DrawSubscriptionToggle(hypeTrains, "Hype Trains (channel:read:hype_train)");
            DrawSubscriptionToggle(redemptions, "Channel point redemptions (channel:manage:redemptions)");

            GUILayout.Space(6);
            GUILayout.BeginHorizontal();
            GUILayout.Label("Event feed (newest first)", SampleGUI.SectionStyle);
            GUILayout.FlexibleSpace();
            if (SampleGUI.Button("Clear feed", feed.Count > 0))
                SampleGUI.Defer(feed.Clear);
            GUILayout.EndHorizontal();

            GUILayout.BeginVertical(GUI.skin.box);
            if (feed.Count == 0)
                GUILayout.Label("No events yet.", SampleGUI.MutedStyle);
            foreach (var line in feed)
                GUILayout.Label(line, SampleGUI.WrapStyle);
            GUILayout.EndVertical();
        }

        void DrawSubscriptionToggle<T>(EventSubscription<T> subscription, string label)
        {
            // Draw from the state at the start of this event; changes apply on the next frame.
            var state = subscription.State;

            GUILayout.BeginHorizontal();
            var wasEnabled = GUI.enabled;
            GUI.enabled = wasEnabled && App.IsLoggedIn;
            bool wanted = GUILayout.Toggle(subscription.IsOn, "  " + label, GUILayout.Width(SampleGUI.LabelWidth + 260));
            GUI.enabled = wasEnabled;

            if (wanted && !subscription.IsOn)
                SampleGUI.Defer(() => subscription.Start(Api));
            else if (!wanted && subscription.IsOn)
                SampleGUI.Defer(() => subscription.Stop());

            if (state == EventSubscription<T>.SubscriptionState.Starting)
                GUILayout.Label("connecting ...", SampleGUI.MutedStyle);
            else if (state == EventSubscription<T>.SubscriptionState.Listening)
                GUILayout.Label("listening", SampleGUI.StyleFor(SampleLogLevel.Success));
            GUILayout.EndHorizontal();
        }

        void AddToFeed(string line)
        {
            feed.Insert(0, $"{DateTime.Now:HH:mm:ss}   {line}");
            if (feed.Count > MaxFeedEntries)
                feed.RemoveRange(MaxFeedEntries, feed.Count - MaxFeedEntries);
            SampleLog.Info("Event: " + line);
        }
    }
}
