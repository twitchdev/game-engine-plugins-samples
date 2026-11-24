using System;
using TwitchSDK;

namespace TwitchSample
{
    /// <summary>
    /// Keeps one EventSub stream open and hands each event to a callback until stopped.
    /// </summary>
    /// <remarks>
    /// Start and Stop run on the main thread, and every await resumes there, so the callback can
    /// update the UI directly. A generation counter makes a stopped (or restarted) loop exit quietly
    /// instead of delivering late events.
    /// </remarks>
    public sealed class EventSubscription<T>
    {
        public enum SubscriptionState
        {
            Off,
            Starting,
            Listening,
        }

        public string Name { get; }
        public SubscriptionState State { get; private set; } = SubscriptionState.Off;
        public bool IsOn => State != SubscriptionState.Off;

        readonly Func<TwitchSDKApi, GameTask<EventStream<T>>> subscribe;
        readonly Action<T> onEvent;
        EventStream<T> stream;
        int generation;

        public EventSubscription(string name, Func<TwitchSDKApi, GameTask<EventStream<T>>> subscribe, Action<T> onEvent)
        {
            Name = name;
            this.subscribe = subscribe;
            this.onEvent = onEvent;
        }

        public async void Start(TwitchSDKApi api)
        {
            if (State != SubscriptionState.Off || api == null)
                return;

            int myGeneration = ++generation;
            State = SubscriptionState.Starting;
            EventStream<T> opened = null;
            try
            {
                opened = await subscribe(api);
                if (myGeneration != generation)
                    return; // stopped while subscribing

                stream = opened;
                State = SubscriptionState.Listening;
                SampleLog.Success($"Listening for {Name}.");

                while (myGeneration == generation)
                {
                    var e = await opened.WaitForEvent();
                    if (myGeneration != generation)
                        break;
                    onEvent(e);
                }
            }
            catch (Exception ex)
            {
                if (myGeneration == generation)
                    SampleLog.Error($"{Name}: {TwitchSampleApp.Describe(ex)}");
            }
            finally
            {
                if (myGeneration == generation)
                {
                    stream = null;
                    State = SubscriptionState.Off;
                }
                // Dispose closes the EventSub subscription; it's safe to call more than once.
                opened?.Dispose();
            }
        }

        public void Stop(bool log = true)
        {
            if (State == SubscriptionState.Off)
                return;

            generation++;
            var current = stream;
            stream = null;
            State = SubscriptionState.Off;
            current?.Dispose();
            if (log)
                SampleLog.Info($"Stopped listening for {Name}.");
        }
    }
}
