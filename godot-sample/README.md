# Twitch SDK Godot samples

Two copies of the same sample, one in **GDScript** (`gdscript/`) and one in **C#** (`csharp/`). They have
the same pages and look as the Unity (`unity-sample`) and Unreal (`unreal-cpp-sample`) samples:

| Page | What it shows |
|------|---------------|
| Authorization | Enter your Client ID, choose scopes, log in with the device code flow, see your user info, log out. |
| Polls | Create a poll from your own question and choices, watch the votes live, end or cancel it. |
| Predictions | Create a prediction, watch the bets, lock it, resolve it to an outcome, or cancel it. |
| Drops | Enter your reward (benefit) IDs, check for Drops, grant and fulfill matching ones, fulfill by ID. |
| General | Users, streams, subscription check, channel info, stream markers, clips, Bits leaderboard, channel point rewards, and live EventSub events. |

A log panel at the bottom shows every result, and optionally the core library's debug messages.

## Running a sample

1. Build the addon: `TwitchSDK-Godot\build.bat`.
2. Copy it into both samples: `import-plugin.bat` (in this folder). Re-run it after rebuilding the addon.
3. Open `gdscript/project.godot` or `csharp/project.godot` in Godot 4.5+ (the C# sample needs the .NET
   edition of Godot) and press Play.
4. On the Authorization page, paste your Client ID and press **Use this Client ID**, then **Log in with Twitch**.

Register the application at [dev.twitch.tv](https://dev.twitch.tv/console/apps) with the OAuth
Redirect URL `http://localhost` and the Client Type **Public**.

The sample remembers your Client ID in `user://twitch_sample.cfg`, and the plugin keeps your login in
`user://twitch-twitchsdk-authorization.json`, so the next run starts logged in. You can also set the
Client ID in Project Settings > Twitch > Auth instead.

To test events without real viewers, enable Project Settings > Twitch > Debug > Use EventSub Proxy, restart,
and send test events with the Twitch CLI (see `core/docs/eventsubproxy.md`).

## Layout

| GDScript | C# | Purpose |
|----------|----|---------|
| `main.gd` | `Scripts/Main.cs` | Checks that the addon is installed, then starts the sample. |
| `scripts/sample_app.gd` | `Scripts/SampleApp.cs` | Header, tabs, log; runs API calls for the buttons; Client ID and login state. |
| `scripts/sample_ui.gd` | `Scripts/SampleUI.cs` | Theme and layout helpers. |
| `scripts/event_subscription.gd` | `Scripts/EventSubscription.cs` | Keeps an EventSub stream open for a toggle on the General page. |
| `scripts/pages/*.gd` | `Scripts/Pages/*.cs` | One file per page. |

## Smoke test

Both samples have a headless self-test that shows every page and checks the input validation (no login needed):

```
godot --headless --path gdscript -- --self-test
godot --headless --path csharp -- --self-test        (after: dotnet build csharp\TwitchSdkCSharpSample.csproj)
```

Run it with a window and `--screenshots=<folder>` to save a picture of every page.
