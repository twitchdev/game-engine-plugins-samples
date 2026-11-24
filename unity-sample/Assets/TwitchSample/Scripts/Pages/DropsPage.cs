using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using TwitchSDK.Interop;
using UnityEngine;

namespace TwitchSample
{
    /// <summary>
    /// Check the logged-in user's Drops, grant the rewards this "game" knows about, and mark them fulfilled.
    /// </summary>
    public sealed class DropsPage : SamplePage
    {
        static readonly string[] StatusOptions = { "Claimed (not granted yet)", "Fulfilled", "All" };
        static readonly DropFulfillmentStatus[] StatusValues =
        {
            DropFulfillmentStatus.Claimed,
            DropFulfillmentStatus.Fulfilled,
            DropFulfillmentStatus.Unknown, // Unknown means "no filter" in the core library
        };

        string gameId = string.Empty;
        int statusIndex;
        string rewardIds = string.Empty;
        string entitlementIds = string.Empty;

        DropEntitlement[] entitlements;
        string lastQueryDescription;

        public override string Title => "Drops";

        public override void ReleaseApiResources()
        {
            entitlements = null;
            lastQueryDescription = null;
        }

        public override void Draw()
        {
            DrawConnectionHint();
            DrawRewards();
            DrawQuery();
            DrawResults();
            DrawManualFulfill();
        }

        void DrawRewards()
        {
            SampleGUI.Section("Your reward IDs",
                "The benefit IDs of the rewards in your Drops campaign (Twitch developer console > Drops > Rewards). " +
                "The sample treats these as the items your game knows how to grant. Separate several IDs with commas.");
            rewardIds = SampleGUI.TextField("Reward (benefit) IDs", rewardIds);

            SampleGUI.KeyValue("Rewards entered", SampleGUI.SplitList(rewardIds).Length.ToString());
        }

        void DrawQuery()
        {
            SampleGUI.Section("Check for Drops",
                "Lists Drops the logged-in user earned for games owned by your Client ID's organization. " +
                "No extra scope is needed. There is no live notification for Drops, so check when it makes sense in your game.");
            gameId = SampleGUI.TextField("Game ID (optional)", gameId, "Leave blank to include all of your games.");
            statusIndex = SampleGUI.Selector("Show", statusIndex, StatusOptions);

            GUILayout.BeginHorizontal();
            GUILayout.Space(SampleGUI.LabelWidth + 4);
            if (SampleGUI.ActionButton(App, "Check for Drops"))
                App.Run("Check for Drops", QueryAsync);

            bool canGrant = entitlements != null && SampleGUI.SplitList(rewardIds).Length > 0;
            if (SampleGUI.ActionButton(App, "Grant and fulfill matching rewards", canGrant))
                GrantMatching();
            GUILayout.EndHorizontal();
        }

        async Task QueryAsync()
        {
            var status = StatusValues[statusIndex];
            var game = SampleGUI.NullIfBlank(gameId);
            var result = await Api.GetDropEntitlements(game, status);

            entitlements = result.Entitlements ?? Array.Empty<DropEntitlement>();
            lastQueryDescription = $"{StatusOptions[statusIndex]}, {(game == null ? "all games" : "game " + game)}, checked at {DateTime.Now:HH:mm:ss}";
            SampleLog.Success($"Found {entitlements.Length} Drop entitlement(s).");
        }

        void GrantMatching()
        {
            var known = new HashSet<string>(SampleGUI.SplitList(rewardIds));
            var matches = entitlements
                .Where(e => e.FulfillmentStatus == DropFulfillmentStatus.Claimed && known.Contains(e.BenefitId))
                .ToArray();

            if (matches.Length == 0)
            {
                SampleLog.Info("None of the claimed Drops match your reward IDs, so there is nothing to grant.");
                return;
            }

            App.Run("Grant and fulfill matching rewards", async () =>
            {
                // A real game adds each item to the player's inventory here, and only fulfills the
                // entitlements it actually granted so the rest are retried next time.
                foreach (var e in matches)
                    SampleLog.Info($"Granting the in-game item for reward {e.BenefitId} (entitlement {e.Id}).");

                await Api.FulfillDropEntitlements(matches);
                SampleLog.Success($"Marked {matches.Length} entitlement(s) fulfilled.");
                await QueryAsync();
            });
        }

        void DrawResults()
        {
            SampleGUI.Section("Results");

            if (entitlements == null)
            {
                GUILayout.Label("Press Check for Drops to load the logged-in user's entitlements.", SampleGUI.MutedStyle);
                return;
            }

            GUILayout.Label(lastQueryDescription, SampleGUI.MutedStyle);
            if (entitlements.Length == 0)
            {
                GUILayout.Label("No entitlements found. If you expected some, check the filters and that the Drops " +
                                "campaign belongs to the organization that owns your Client ID.", SampleGUI.WrapStyle);
                return;
            }

            var known = new HashSet<string>(SampleGUI.SplitList(rewardIds));
            foreach (var e in entitlements)
            {
                GUILayout.BeginVertical(GUI.skin.box);
                SampleGUI.KeyValue("Entitlement ID", e.Id);
                SampleGUI.KeyValue("Reward (benefit) ID", e.BenefitId + (known.Contains(e.BenefitId) ? "   (matches your reward IDs)" : ""));
                SampleGUI.KeyValue("Status", e.FulfillmentStatus.ToString());
                SampleGUI.KeyValue("Game ID", e.GameId);
                SampleGUI.KeyValue("Granted", e.Timestamp);
                SampleGUI.KeyValue("Last updated", e.LastUpdated);

                var action = "Fulfill " + e.Id;
                GUILayout.BeginHorizontal();
                GUILayout.Space(SampleGUI.LabelWidth + 4);
                if (SampleGUI.ActionButton(App, action, e.FulfillmentStatus == DropFulfillmentStatus.Claimed))
                {
                    var id = e.Id;
                    App.Run(action, async () =>
                    {
                        await Api.FulfillDropEntitlements(id);
                        SampleLog.Success($"Marked entitlement {id} fulfilled.");
                        await QueryAsync();
                    });
                }
                GUILayout.EndHorizontal();
                GUILayout.EndVertical();
            }
        }

        void DrawManualFulfill()
        {
            SampleGUI.Section("Fulfill by entitlement ID",
                "Marks specific entitlements fulfilled without checking first. Separate several IDs with commas.");
            entitlementIds = SampleGUI.TextField("Entitlement IDs", entitlementIds);

            GUILayout.BeginHorizontal();
            GUILayout.Space(SampleGUI.LabelWidth + 4);
            var ids = SampleGUI.SplitList(entitlementIds);
            if (SampleGUI.ActionButton(App, "Fulfill these entitlements", ids.Length > 0))
            {
                App.Run("Fulfill these entitlements", async () =>
                {
                    await Api.FulfillDropEntitlements(ids);
                    SampleLog.Success($"Sent {ids.Length} entitlement(s) to Twitch to mark fulfilled. Check for Drops to confirm.");
                });
            }
            GUILayout.EndHorizontal();
        }
    }
}
