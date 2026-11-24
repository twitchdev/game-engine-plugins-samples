using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using Godot;
using TwitchSDK;

namespace TwitchSample
{
    /// <summary>
    /// Check the logged-in user's Drops, grant the rewards this "game" knows about, and mark them fulfilled.
    /// </summary>
    public partial class DropsPage : SamplePage
    {
        static readonly string[] StatusOptions = { "Claimed (not granted yet)", "Fulfilled", "All" };
        static readonly DropFulfillmentStatus[] StatusValues =
        {
            DropFulfillmentStatus.Claimed,
            DropFulfillmentStatus.Fulfilled,
            DropFulfillmentStatus.Unknown, // Unknown means "no filter" in the core library
        };

        LineEdit rewardIdsEdit;
        Label rewardsEntered;
        LineEdit gameIdEdit;
        OptionButton statusSelect;
        LineEdit entitlementIdsEdit;
        VBoxContainer results;
        TwitchDropEntitlement[] entitlements;
        string lastQuery;

        public override string Title => "Drops";

        protected override void Build()
        {
            AddConnectionHint();

            SampleUI.AddSection(this, "Your reward IDs",
                "The benefit IDs of the rewards in your Drops campaign (Twitch developer console > Drops > Rewards). " +
                "The sample treats these as the items your game knows how to grant. Separate several IDs with commas.");
            rewardIdsEdit = SampleUI.AddTextRow(this, "Reward (benefit) IDs");
            rewardsEntered = SampleUI.AddKeyValue(this, "Rewards entered", "0");
            rewardIdsEdit.TextChanged += text =>
            {
                rewardsEntered.Text = SampleUI.SplitList(text).Length.ToString();
                ShowResults();
            };

            SampleUI.AddSection(this, "Check for Drops",
                "Lists Drops the logged-in user earned for games owned by your Client ID's organization. " +
                "No extra scope is needed. There is no live notification for Drops, so check when it makes sense in your game.");
            gameIdEdit = SampleUI.AddTextRow(this, "Game ID (optional)", "", "Leave blank to include all of your games.");
            statusSelect = SampleUI.AddSelector(this, "Show", StatusOptions);
            var buttons = SampleUI.AddIndented(this);
            App.ActionButton(buttons, "Check for Drops", () => App.Run("Check for Drops", QueryAsync));
            App.ActionButton(buttons, "Grant and fulfill matching rewards", GrantMatching,
                () => entitlements != null && SampleUI.SplitList(rewardIdsEdit.Text).Length > 0);

            SampleUI.AddSection(this, "Results");
            results = new VBoxContainer();
            AddChild(results);
            ShowResults();

            SampleUI.AddSection(this, "Fulfill by entitlement ID",
                "Marks specific entitlements fulfilled without checking first. Separate several IDs with commas.");
            entitlementIdsEdit = SampleUI.AddTextRow(this, "Entitlement IDs");
            App.ActionButton(SampleUI.AddIndented(this), "Fulfill these entitlements", FulfillManual,
                () => SampleUI.SplitList(entitlementIdsEdit.Text).Length > 0);
        }

        async Task QueryAsync()
        {
            var index = statusSelect.Selected;
            var game = gameIdEdit.Text.Trim();
            var result = await Twitch.GetDropEntitlements(game, StatusValues[index]);
            entitlements = result.Entitlements ?? Array.Empty<TwitchDropEntitlement>();
            lastQuery = $"{StatusOptions[index]}, {(game.Length == 0 ? "all games" : "game " + game)}, checked at {DateTime.Now:HH:mm:ss}";
            ShowResults();
            App.Success($"Found {entitlements.Length} Drop entitlement(s).");
        }

        void GrantMatching()
        {
            var known = new HashSet<string>(SampleUI.SplitList(rewardIdsEdit.Text));
            var matches = entitlements
                .Where(e => e.FulfillmentStatus == DropFulfillmentStatus.Claimed && known.Contains(e.BenefitId))
                .ToArray();
            if (matches.Length == 0)
            {
                App.Info("None of the claimed Drops match your reward IDs, so there is nothing to grant.");
                return;
            }

            App.Run("Grant and fulfill matching rewards", async () =>
            {
                // A real game adds each item to the player's inventory here, and only fulfills the
                // entitlements it actually granted so the rest are retried next time.
                foreach (var e in matches)
                    App.Info($"Granting the in-game item for reward {e.BenefitId} (entitlement {e.Id}).");
                await Twitch.FulfillDropEntitlements(matches);
                App.Success($"Marked {matches.Length} entitlement(s) fulfilled.");
                await QueryAsync();
            });
        }

        void ShowResults()
        {
            SampleUI.Clear(results);
            if (entitlements == null)
            {
                results.AddChild(SampleUI.Muted("Press Check for Drops to load the logged-in user's entitlements."));
                return;
            }
            results.AddChild(SampleUI.Muted(lastQuery));
            if (entitlements.Length == 0)
            {
                results.AddChild(SampleUI.MakeLabel("No entitlements found. If you expected some, check the filters and that the Drops " +
                                                    "campaign belongs to the organization that owns your Client ID.", SampleUI.TextColor, 0, false, true));
                return;
            }

            var known = new HashSet<string>(SampleUI.SplitList(rewardIdsEdit.Text));
            foreach (var e in entitlements)
            {
                var box = SampleUI.AddPanel(results);
                SampleUI.AddKeyValue(box, "Entitlement ID", e.Id);
                SampleUI.AddKeyValue(box, "Reward (benefit) ID", e.BenefitId + (known.Contains(e.BenefitId) ? "   (matches your reward IDs)" : ""));
                SampleUI.AddKeyValue(box, "Status", e.FulfillmentStatus.ToString());
                SampleUI.AddKeyValue(box, "Game ID", e.GameId);
                SampleUI.AddKeyValue(box, "Granted", e.Timestamp);
                SampleUI.AddKeyValue(box, "Last updated", e.LastUpdated);
                var action = "Fulfill " + e.Id;
                var id = e.Id;
                App.ActionButton(SampleUI.AddIndented(box), action, () => App.Run(action, async () =>
                {
                    await Twitch.FulfillDropEntitlements(new[] { id });
                    App.Success($"Marked entitlement {id} fulfilled.");
                    await QueryAsync();
                }), () => e.FulfillmentStatus == DropFulfillmentStatus.Claimed);
            }
        }

        void FulfillManual()
        {
            var ids = SampleUI.SplitList(entitlementIdsEdit.Text);
            App.Run("Fulfill these entitlements", async () =>
            {
                await Twitch.FulfillDropEntitlements(ids);
                App.Success($"Sent {ids.Length} entitlement(s) to Twitch to mark fulfilled. Check for Drops to confirm.");
            });
        }
    }
}
