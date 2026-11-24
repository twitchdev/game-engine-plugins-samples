using System.Threading;
using System.Threading.Tasks;
using TwitchSDK;
using TwitchSDK.Interop;
using UnityEngine;

namespace TwitchSample
{
    /// <summary>
    /// A <see cref="TwitchSDKApi"/> created from a Client ID typed in at runtime.
    /// </summary>
    /// <remarks>
    /// The plugin's own <c>Twitch.API</c> singleton reads its Client ID from the project's Twitch
    /// settings asset, which can't be changed from game code. To let users enter their own Client ID
    /// on the Authorization page, the sample creates its own instance instead. The PAL below mirrors
    /// the plugin's Unity PAL: tokens are stored under <see cref="Application.persistentDataPath"/>
    /// and core log messages go to the sample log.
    /// </remarks>
    public sealed class SampleTwitchApi : TwitchSDKApi
    {
        public string ClientId { get; }
        public bool UsesEventSubProxy { get; }

        public SampleTwitchApi(string clientId, bool useEventSubProxy)
            : base(clientId, useEventSubProxy)
        {
            ClientId = clientId;
            UsesEventSubProxy = useEventSubProxy;
        }

        // Called from the base constructor, which the sample always runs on the main thread,
        // so reading Application.persistentDataPath here is safe.
        protected override PlatformAbstractionLayer CreatePAL() => new SamplePAL(Application.persistentDataPath);

        sealed class SamplePAL : ManagedPAL
        {
            readonly string BasePath;

            public SamplePAL(string basePath)
            {
                BasePath = basePath;
            }

            protected override string HttpUserAgent => "Twitch-Route-66-Unity";

            protected override Task<string> GetFileIOBasePath(CancellationToken _) => Task.FromResult(BasePath);

            protected override Task Log(LogRequest req)
            {
                var message = "[core] " + req.Message;
                switch (req.Level)
                {
                    case LogLevel.Debug:
                        if (SampleLog.ShowCoreDebugMessages)
                            SampleLog.Debug(message);
                        break;
                    case LogLevel.Warning:
                        SampleLog.Warning(message);
                        break;
                    case LogLevel.Error:
                        SampleLog.Error(message);
                        break;
                    default:
                        SampleLog.Info(message);
                        break;
                }
                return Task.CompletedTask;
            }
        }
    }
}
