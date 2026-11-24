using System;
using System.Collections.Generic;
using System.Linq;
using Godot;
using TwitchSDK;

namespace TwitchSample
{
    /// <summary>
    /// Everything that doesn't belong to another page: users, streams, subscriptions, channel edits,
    /// stream markers, clips, the Bits leaderboard, channel point rewards, and live events.
    /// </summary>
    public partial class GeneralPage : SamplePage
    {
        const int MaxFeedEntries = 50;
        static readonly string[] LeaderboardPeriods = { "day", "week", "month", "year", "all" };

        // Users
        LineEdit loginEdit;
        LineEdit userIdEdit;
        VBoxContainer userBox;
        // Streams
        VBoxContainer myStreamBox;
        LineEdit streamLoginsEdit;
        VBoxContainer streamsBox;
        // Subscription check
        LineEdit broadcasterIdEdit;
        VBoxContainer subscriptionBox;
        // Channel information
        LineEdit channelTitleEdit;
        LineEdit channelGameEdit;
        LineEdit channelLanguageEdit;
        LineEdit channelTagsEdit;
        CheckBox clearTagsCheck;
        // Markers and clips
        LineEdit markerEdit;
        VBoxContainer markerBox;
        CheckBox clipDelayCheck;
        VBoxContainer clipBox;
        // Bits leaderboard
        LineEdit leaderboardCountEdit;
        OptionButton leaderboardPeriod;
        VBoxContainer leaderboardBox;
        // Channel point rewards
        LineEdit rewardTitleEdit;
        LineEdit rewardCostEdit;
        LineEdit rewardPromptEdit;
        CheckBox rewardInputCheck;
        VBoxContainer redemptionsBox;
        readonly List<TwitchCustomRewardEvent> pendingRedemptions = new List<TwitchCustomRewardEvent>();
        // Live events
        readonly List<IEventSubscription> subscriptions = new List<IEventSubscription>();
        readonly List<string> feed = new List<string>();
        VBoxContainer feedBox;

        public override string Title => "General";

        protected override void Build()
        {
            AddConnectionHint();
            BuildUsers();
            BuildStreams();
            BuildSubscriptionCheck();
            BuildChannelInfo();
            BuildMarkersAndClips();
            BuildBitsLeaderboard();
            BuildRewards();
            BuildEvents();
            App.AuthStateChanged += StopEventsIfLoggedOut;
        }

        public override void _ExitTree()
        {
            if (App != null)
                App.AuthStateChanged -= StopEventsIfLoggedOut;
            foreach (var s in subscriptions)
                s.Stop(log: false);
        }

        /// <summary>Stops live events after a log out so the toggles match what's actually running.</summary>
        void StopEventsIfLoggedOut()
        {
            if (!App.IsLoggedIn)
                foreach (var s in subscriptions)
                    s.Stop();
        }

        // ------------------------------------------------------------------ Users

        void BuildUsers()
        {
            var c = SampleUI.AddFoldout(this, "Users", "Look up any Twitch user by login name or user ID.");
            App.ActionButton(SampleUI.AddIndented(c), "Get my user info", () => Lookup("Get my user info", Twitch.GetMyUserInfo));

            loginEdit = SampleUI.AddTextRow(c, "Login name");
            App.ActionButton(SampleUI.AddIndented(c), "Look up by login", () =>
            {
                var login = loginEdit.Text.Trim();
                Lookup("Look up by login", () => Twitch.GetUserInfoByLoginName(login));
            }, () => loginEdit.Text.Trim().Length > 0);

            userIdEdit = SampleUI.AddTextRow(c, "User ID");
            App.ActionButton(SampleUI.AddIndented(c), "Look up by ID", () =>
            {
                var id = userIdEdit.Text.Trim();
                Lookup("Look up by ID", () => Twitch.GetUserInfoById(id));
            }, () => userIdEdit.Text.Trim().Length > 0);

            userBox = new VBoxContainer();
            c.AddChild(userBox);
        }

        void Lookup(string action, Func<TwitchTask<TwitchUserInfo>> start)
        {
            App.Run(action, async () =>
            {
                var user = await start();
                SampleUI.Clear(userBox);
                if (user == null || string.IsNullOrEmpty(user.ChannelId))
                {
                    App.Warning("No user found.");
                    return;
                }
                var box = SampleUI.AddPanel(userBox);
                SampleUI.AddKeyValue(box, "Display name", user.DisplayName);
                SampleUI.AddKeyValue(box, "Login", user.LoginName);
                SampleUI.AddKeyValue(box, "User ID", user.ChannelId);
                SampleUI.AddKeyValue(box, "Broadcaster type", string.IsNullOrEmpty(user.BroadcasterType) ? "(none)" : user.BroadcasterType);
                SampleUI.AddKeyValue(box, "Description", user.Description);
                SampleUI.AddKeyValue(box, "Created", user.CreatedAt);
                SampleUI.AddKeyValue(box, "Profile image", user.ProfileImageUrl);
                var buttons = SampleUI.AddIndented(box);
                SampleUI.AddButton(buttons, "Use this ID for the subscription check", () => broadcasterIdEdit.Text = user.ChannelId);
                SampleUI.AddButton(buttons, "Open channel", () => OS.ShellOpen("https://www.twitch.tv/" + user.LoginName));
                App.Success($"Found {user.DisplayName} (user ID {user.ChannelId}).");
            });
        }

        // ------------------------------------------------------------------ Streams

        void BuildStreams()
        {
            var c = SampleUI.AddFoldout(this, "Streams", "See your own stream and find out which channels are live.");
            App.ActionButton(SampleUI.AddIndented(c), "Get my stream info", () => App.Run("Get my stream info", async () =>
            {
                var stream = await Twitch.GetMyStreamInfo();
                SampleUI.Clear(myStreamBox);
                var box = SampleUI.AddPanel(myStreamBox);
                if (IsLive(stream))
                {
                    ShowStream(box, stream);
                    App.Success("You are live: " + stream.Title);
                }
                else
                {
                    box.AddChild(SampleUI.Muted("Your channel is offline."));
                    App.Success("Your channel is offline.");
                }
            }));
            myStreamBox = new VBoxContainer();
            c.AddChild(myStreamBox);

            streamLoginsEdit = SampleUI.AddTextRow(c, "Channel logins", "", "Separate several logins with commas (up to 100).");
            App.ActionButton(SampleUI.AddIndented(c), "Find live streams", () =>
            {
                var logins = SampleUI.SplitList(streamLoginsEdit.Text);
                App.Run("Find live streams", async () =>
                {
                    var result = await Twitch.QueryStreams(new TwitchStreamQuery { UserLogins = logins, First = Math.Min(logins.Length, 100) });
                    var streams = result.Streams ?? Array.Empty<TwitchStreamInfo>();
                    SampleUI.Clear(streamsBox);
                    if (streams.Length == 0)
                        streamsBox.AddChild(SampleUI.Muted("None of those channels are live."));
                    foreach (var stream in streams)
                        ShowStream(SampleUI.AddPanel(streamsBox), stream);
                    App.Success($"{streams.Length} of {logins.Length} channel(s) are live.");
                });
            }, () => SampleUI.SplitList(streamLoginsEdit.Text).Length > 0);
            streamsBox = new VBoxContainer();
            c.AddChild(streamsBox);
        }

        static bool IsLive(TwitchStreamInfo stream) => stream != null && !string.IsNullOrEmpty(stream.Id);

        static void ShowStream(Control box, TwitchStreamInfo stream)
        {
            SampleUI.AddKeyValue(box, "Channel", stream.UserName);
            SampleUI.AddKeyValue(box, "Title", stream.Title);
            SampleUI.AddKeyValue(box, "Category", stream.GameName);
            SampleUI.AddKeyValue(box, "Viewers", stream.ViewerCount);
            SampleUI.AddKeyValue(box, "Started", stream.StartedAt);
            SampleUI.AddKeyValue(box, "Language", stream.Language);
            if (stream.Tags != null && stream.Tags.Length > 0)
                SampleUI.AddKeyValue(box, "Tags", string.Join(", ", stream.Tags));
        }

        // ------------------------------------------------------------------ Subscription check

        void BuildSubscriptionCheck()
        {
            var c = SampleUI.AddFoldout(this, "Subscription check",
                "Checks whether the logged-in user subscribes to a channel. Find a channel's user ID with the Users section above.");
            broadcasterIdEdit = SampleUI.AddTextRow(c, "Broadcaster user ID");
            App.ActionButton(SampleUI.AddIndented(c), "Check my subscription", () =>
            {
                var id = broadcasterIdEdit.Text.Trim();
                App.Run("Check my subscription", async () =>
                {
                    var result = await Twitch.CheckUserSubscription(id);
                    SampleUI.Clear(subscriptionBox);
                    var box = SampleUI.AddPanel(subscriptionBox);
                    SampleUI.AddKeyValue(box, "Broadcaster ID", id);
                    SampleUI.AddKeyValue(box, "Subscribed", result.IsSubscribed ? "Yes" : "No");
                    if (result.IsSubscribed)
                    {
                        SampleUI.AddKeyValue(box, "Tier", SampleUI.TierName(result.Tier));
                        SampleUI.AddKeyValue(box, "Gift", result.IsGift ? $"Yes, from {result.GifterName}" : "No");
                        App.Success($"You subscribe to {id} at tier {SampleUI.TierName(result.Tier)}.");
                    }
                    else
                    {
                        App.Success($"You don't subscribe to {id}.");
                    }
                });
            }, () => broadcasterIdEdit.Text.Trim().Length > 0);
            subscriptionBox = new VBoxContainer();
            c.AddChild(subscriptionBox);
        }

        // ------------------------------------------------------------------ Channel information

        void BuildChannelInfo()
        {
            var c = SampleUI.AddFoldout(this, "Channel information", "Updates your channel. Fill in only the fields you want to change.");
            channelTitleEdit = SampleUI.AddTextRow(c, "Stream title");
            channelGameEdit = SampleUI.AddTextRow(c, "Category (game) ID", "", "A numeric category ID, e.g. 509658 for Just Chatting.");
            channelLanguageEdit = SampleUI.AddTextRow(c, "Language", "", "A two-letter code such as en or de.");
            channelTagsEdit = SampleUI.AddTextRow(c, "Tags", "", "Separate tags with commas. Tags can't contain spaces.");
            clearTagsCheck = SampleUI.AddCheck(c, "Remove all tags");

            var buttons = SampleUI.AddIndented(c);
            App.ActionButton(buttons, "Load current values", () => App.Run("Load current values", async () =>
            {
                var stream = await Twitch.GetMyStreamInfo();
                if (!IsLive(stream))
                {
                    App.Info("Your channel is offline, so there are no current values to load. Enter them manually.");
                    return;
                }
                channelTitleEdit.Text = stream.Title ?? "";
                channelGameEdit.Text = stream.GameId ?? "";
                channelLanguageEdit.Text = stream.Language ?? "";
                channelTagsEdit.Text = stream.Tags == null ? "" : string.Join(", ", stream.Tags);
                App.Success("Loaded the current channel values.");
            }));
            App.ActionButton(buttons, "Update channel", UpdateChannel);
        }

        void UpdateChannel()
        {
            var tagList = SampleUI.SplitList(channelTagsEdit.Text);
            // Tags are only changed when some are given, or when they should all be removed.
            var tags = clearTagsCheck.ButtonPressed ? Array.Empty<string>() : tagList;
            var request = new TwitchModifyChannelInfoRequest
            {
                Title = channelTitleEdit.Text.Trim(),
                GameId = channelGameEdit.Text.Trim(),
                Language = channelLanguageEdit.Text.Trim(),
                Tags = tags,
                ForceUpdateTags = clearTagsCheck.ButtonPressed || tagList.Length > 0,
            };
            if (request.Title.Length == 0 && request.GameId.Length == 0 && request.Language.Length == 0 && !request.ForceUpdateTags)
            {
                App.Error("Fill in at least one field to change.");
                return;
            }
            App.Run("Update channel", async () =>
            {
                await Twitch.ModifyChannelInformation(request);
                App.Success("Channel information updated.");
            });
        }

        // ------------------------------------------------------------------ Markers and clips

        void BuildMarkersAndClips()
        {
            var c = SampleUI.AddFoldout(this, "Stream markers and clips", "Both need your channel to be live.");
            markerEdit = SampleUI.AddTextRow(c, "Marker description", "Sample marker", "Up to 140 characters.");
            App.ActionButton(SampleUI.AddIndented(c), "Create stream marker", () =>
            {
                var description = markerEdit.Text.Trim();
                App.Run("Create stream marker", async () =>
                {
                    var marker = await Twitch.CreateStreamMarker(description);
                    SampleUI.Clear(markerBox);
                    var box = SampleUI.AddPanel(markerBox);
                    SampleUI.AddKeyValue(box, "Marker ID", marker.Id);
                    SampleUI.AddKeyValue(box, "Description", marker.Description);
                    SampleUI.AddKeyValue(box, "Position", marker.PositionSeconds + " seconds");
                    SampleUI.AddKeyValue(box, "Created", marker.CreatedAt);
                    App.Success($"Marker created {marker.PositionSeconds} seconds into the stream.");
                });
            });
            markerBox = new VBoxContainer();
            c.AddChild(markerBox);

            SampleUI.AddSpacer(c, 6);
            clipDelayCheck = SampleUI.AddCheck(c, "Account for the stream delay when clipping");
            App.ActionButton(SampleUI.AddIndented(c), "Create clip", () =>
            {
                var delay = clipDelayCheck.ButtonPressed;
                App.Run("Create clip", async () =>
                {
                    App.Info("Creating a clip. Twitch takes a few seconds to process it ...");
                    var clip = await Twitch.CreateClip(delay);
                    SampleUI.Clear(clipBox);
                    var box = SampleUI.AddPanel(clipBox);
                    SampleUI.AddKeyValue(box, "Clip ID", clip.Id);
                    SampleUI.AddKeyValue(box, "URL", clip.Url);
                    SampleUI.AddKeyValue(box, "Duration", clip.Duration + " seconds");
                    var open = SampleUI.AddButton(SampleUI.AddIndented(box), "Open clip", () => OS.ShellOpen(clip.Url));
                    open.Disabled = string.IsNullOrEmpty(clip.Url);
                    App.Success("Clip created: " + clip.Url);
                });
            });
            clipBox = new VBoxContainer();
            c.AddChild(clipBox);
        }

        // ------------------------------------------------------------------ Bits leaderboard

        void BuildBitsLeaderboard()
        {
            var c = SampleUI.AddFoldout(this, "Bits leaderboard", "The top cheerers on your channel.");
            leaderboardCountEdit = SampleUI.AddTextRow(c, "Entries", "10", "1 to 100.");
            leaderboardPeriod = SampleUI.AddSelector(c, "Period", LeaderboardPeriods, 4);
            App.ActionButton(SampleUI.AddIndented(c), "Get Bits leaderboard", () =>
            {
                if (!SampleUI.TryParseInt(App, leaderboardCountEdit.Text, "Entries", 1, 100, out var count))
                    return;
                var period = LeaderboardPeriods[leaderboardPeriod.Selected];
                App.Run("Get Bits leaderboard", async () =>
                {
                    var leaderboard = await Twitch.GetBitsLeaderboard(new TwitchBitsLeaderboardRequest { Count = count, Period = period });
                    var entries = leaderboard.Data ?? Array.Empty<TwitchBitsLeaderboardEntry>();
                    SampleUI.Clear(leaderboardBox);
                    var box = SampleUI.AddPanel(leaderboardBox);
                    if (entries.Length == 0)
                        box.AddChild(SampleUI.Muted("Nobody has cheered in this period."));
                    foreach (var entry in entries)
                        SampleUI.AddKeyValue(box, $"#{entry.Rank}  {entry.UserName}", $"{SampleUI.Number(entry.Score)} bits");
                    App.Success($"Loaded {entries.Length} leaderboard entries.");
                });
            });
            leaderboardBox = new VBoxContainer();
            c.AddChild(leaderboardBox);
        }

        // ------------------------------------------------------------------ Channel point rewards

        void BuildRewards()
        {
            var c = SampleUI.AddFoldout(this, "Channel point rewards",
                "Creates a custom reward on your channel. Replacing rewards affects only rewards made with this " +
                "Client ID. Turn on channel point redemptions under Live events to see and resolve redemptions.");
            rewardTitleEdit = SampleUI.AddTextRow(c, "Reward title", "Sample reward", "Up to 45 characters, unique on your channel.");
            rewardCostEdit = SampleUI.AddTextRow(c, "Cost (channel points)", "100");
            rewardPromptEdit = SampleUI.AddTextRow(c, "Prompt", "Redeemed from the Twitch SDK Godot sample");
            rewardInputCheck = SampleUI.AddCheck(c, "Viewers must enter text when redeeming");

            var buttons = SampleUI.AddIndented(c);
            App.ActionButton(buttons, "Create reward", CreateReward);
            App.ActionButton(buttons, "Remove sample rewards", () => App.Run("Remove sample rewards", async () =>
            {
                await Twitch.ReplaceCustomRewards(Array.Empty<TwitchCustomRewardDefinition>());
                App.Success("Removed the rewards created with this Client ID.");
            }));

            redemptionsBox = new VBoxContainer();
            c.AddChild(redemptionsBox);
        }

        void CreateReward()
        {
            if (!SampleUI.ValidateText(App, rewardTitleEdit.Text, "Reward title", 45) ||
                !SampleUI.TryParseInt(App, rewardCostEdit.Text, "Cost", 1, int.MaxValue, out var cost))
                return;
            var definition = new TwitchCustomRewardDefinition
            {
                Title = rewardTitleEdit.Text.Trim(),
                Cost = cost,
                Prompt = rewardPromptEdit.Text.Trim(),
                IsEnabled = true,
                IsUserInputRequired = rewardInputCheck.ButtonPressed,
            };
            App.Run("Create reward", async () =>
            {
                await Twitch.ReplaceCustomRewards(new[] { definition });
                App.Success($"Reward \"{definition.Title}\" is now on your channel for {SampleUI.Number(cost)} points.");
            });
        }

        void ShowRedemptions()
        {
            SampleUI.Clear(redemptionsBox);
            if (pendingRedemptions.Count == 0)
                return;
            SampleUI.AddSpacer(redemptionsBox, 6);
            redemptionsBox.AddChild(SampleUI.MakeLabel("Waiting redemptions", Colors.White, 15, true));
            foreach (var redemption in pendingRedemptions)
            {
                var box = SampleUI.AddPanel(redemptionsBox);
                SampleUI.AddKeyValue(box, "Reward", redemption.CustomRewardTitle);
                SampleUI.AddKeyValue(box, "Viewer", redemption.RedeemerName);
                if (!string.IsNullOrEmpty(redemption.UserInput))
                    SampleUI.AddKeyValue(box, "Their text", redemption.UserInput);
                SampleUI.AddKeyValue(box, "Redeemed", redemption.RedeemedAt);
                var buttons = SampleUI.AddIndented(box);
                var fulfill = "Fulfill " + redemption.RedemptionId;
                var refund = "Refund " + redemption.RedemptionId;
                App.ActionButton(buttons, fulfill, () => Resolve(fulfill, redemption, CustomRewardRedemptionState.Fulfilled));
                App.ActionButton(buttons, refund, () => Resolve(refund, redemption, CustomRewardRedemptionState.Canceled));
                SampleUI.AddButton(buttons, "Dismiss", () =>
                {
                    pendingRedemptions.Remove(redemption);
                    ShowRedemptions();
                });
            }
        }

        void Resolve(string action, TwitchCustomRewardEvent redemption, CustomRewardRedemptionState state)
        {
            App.Run(action, async () =>
            {
                await Twitch.ResolveCustomReward(redemption, state);
                pendingRedemptions.Remove(redemption);
                ShowRedemptions();
                App.Success(state == CustomRewardRedemptionState.Fulfilled
                    ? $"Fulfilled {redemption.RedeemerName}'s \"{redemption.CustomRewardTitle}\"."
                    : $"Refunded {redemption.RedeemerName}'s \"{redemption.CustomRewardTitle}\".");
            });
        }

        void OnRedemption(TwitchCustomRewardEvent e)
        {
            AddToFeed($"{e.RedeemerName} redeemed \"{e.CustomRewardTitle}\" for {SampleUI.Number(e.CustomRewardCost)} points." +
                      (string.IsNullOrEmpty(e.UserInput) ? "" : $" They wrote: {e.UserInput}"));
            // Only redemptions still in the queue can be fulfilled or refunded.
            if (e.Status == CustomRewardRedemptionState.Unfulfilled && pendingRedemptions.All(x => x.RedemptionId != e.RedemptionId))
            {
                pendingRedemptions.Add(e);
                ShowRedemptions();
            }
        }

        // ------------------------------------------------------------------ Live events

        void BuildEvents()
        {
            var c = SampleUI.AddFoldout(this, "Live events (EventSub)",
                "Turn on the events you want to see. To test without real viewers, enable the local " +
                "EventSubProxy (Project Settings > Twitch > Debug) and send test events with the Twitch CLI.");

            AddSubscription(c, "Follows (moderator:read:followers)", new EventSubscription<TwitchChannelFollowEvent>(App, "follows",
                Twitch.SubscribeToChannelFollowEvents,
                e => AddToFeed($"{e.UserDisplayName} followed the channel.")));
            AddSubscription(c, "Subscriptions (channel:read:subscriptions)", new EventSubscription<TwitchChannelSubscribeEvent>(App, "subscriptions",
                Twitch.SubscribeToChannelSubscribeEvents,
                e => AddToFeed(e.IsGift
                    ? $"{e.UserDisplayName} received a gifted tier {SampleUI.TierName(e.Tier)} subscription."
                    : $"{e.UserDisplayName} subscribed at tier {SampleUI.TierName(e.Tier)} ({e.CumulativeMonths} months total).")));
            AddSubscription(c, "Cheers (bits:read)", new EventSubscription<TwitchChannelCheerEvent>(App, "cheers",
                Twitch.SubscribeToChannelCheerEvents,
                e => AddToFeed($"{(e.IsAnonymous ? "An anonymous viewer" : e.UserDisplayName)} cheered {SampleUI.Number(e.Bits)} bits: {e.Message}")));
            AddSubscription(c, "Raids (no scope needed)", new EventSubscription<TwitchChannelRaidEvent>(App, "raids",
                Twitch.SubscribeToChannelRaidEvents,
                e => AddToFeed($"{e.FromBroadcasterName} raided {e.ToBroadcasterName} with {SampleUI.Number(e.Viewers)} viewers.")));
            AddSubscription(c, "Hype Trains (channel:read:hype_train)", new EventSubscription<TwitchHypeTrainEvent>(App, "Hype Train updates",
                Twitch.SubscribeToHypeTrainEvents,
                e => AddToFeed(string.IsNullOrEmpty(e.EndedAt)
                    ? $"Hype Train level {e.Level}: {SampleUI.Number(e.Progress)} / {SampleUI.Number(e.Goal)} points."
                    : $"Hype Train ended at level {e.Level} with {SampleUI.Number(e.TotalPoints)} points.")));
            AddSubscription(c, "Channel point redemptions (channel:manage:redemptions)", new EventSubscription<TwitchCustomRewardEvent>(App,
                "channel point redemptions", Twitch.SubscribeToCustomRewardEvents, OnRedemption));

            SampleUI.AddSpacer(c, 6);
            var header = new HBoxContainer();
            var title = SampleUI.MakeLabel("Event feed (newest first)", Colors.White, 15, true);
            title.SizeFlagsHorizontal = SizeFlags.ExpandFill;
            header.AddChild(title);
            App.WatchButton(SampleUI.AddButton(header, "Clear feed", () =>
            {
                feed.Clear();
                ShowFeed();
            }), () => feed.Count > 0);
            c.AddChild(header);
            feedBox = SampleUI.AddPanel(c);
            ShowFeed();
        }

        void AddSubscription(Control parent, string text, IEventSubscription subscription)
        {
            subscriptions.Add(subscription);
            var row = new HBoxContainer();
            var toggle = new CheckBox { Text = text, CustomMinimumSize = new Vector2(SampleUI.LabelWidth + 260, 0) };
            row.AddChild(toggle);
            var state = SampleUI.Muted("");
            row.AddChild(state);
            parent.AddChild(row);

            toggle.Toggled += on =>
            {
                if (on)
                    subscription.Start();
                else
                    subscription.Stop();
            };
            subscription.StateChanged += () =>
            {
                if (!IsInstanceValid(toggle))
                    return;
                toggle.SetPressedNoSignal(subscription.IsOn);
                switch (subscription.State)
                {
                    case SubscriptionState.Starting:
                        state.Text = "connecting ...";
                        state.AddThemeColorOverride("font_color", SampleUI.MutedColor);
                        break;
                    case SubscriptionState.Listening:
                        state.Text = "listening";
                        state.AddThemeColorOverride("font_color", SampleUI.LevelColor(SampleLogLevel.Success));
                        break;
                    default:
                        state.Text = "";
                        break;
                }
            };
            App.WatchButton(toggle, () => App.IsLoggedIn || subscription.IsOn);
        }

        void AddToFeed(string line)
        {
            feed.Insert(0, $"{DateTime.Now:HH:mm:ss}   {line}");
            if (feed.Count > MaxFeedEntries)
                feed.RemoveRange(MaxFeedEntries, feed.Count - MaxFeedEntries);
            ShowFeed();
            App.Info("Event: " + line);
        }

        void ShowFeed()
        {
            SampleUI.Clear(feedBox);
            if (feed.Count == 0)
                feedBox.AddChild(SampleUI.Muted("No events yet."));
            foreach (var line in feed)
                feedBox.AddChild(SampleUI.MakeLabel(line, SampleUI.TextColor, 0, false, true));
        }
    }
}
