using System;
using TwitchSDK;

namespace TwitchSample
{
    public enum SubscriptionState
    {
        Off,
        Starting,
        Listening,
    }

    public interface IEventSubscription
    {
        SubscriptionState State { get; }
        bool IsOn { get; }
        event Action StateChanged;
        void Start();
        void Stop(bool log = true);
    }

    /// <summary>
    /// Keeps one EventSub stream open and hands each event to a callback until stopped.
    /// </summary>
    /// <remarks>
    /// Every await resumes on the main thread, so the callback can update the UI directly. A
    /// generation counter makes a stopped (or restarted) loop exit quietly instead of delivering late events.
    /// </remarks>
    public sealed class EventSubscription<T> : IEventSubscription
    {
        public string Name { get; }
        public SubscriptionState State { get; private set; } = SubscriptionState.Off;
        public bool IsOn => State != SubscriptionState.Off;
        public event Action StateChanged;

        readonly SampleApp app;
        readonly Func<TwitchTask<EventStream<T>>> subscribe;
        readonly Action<T> onEvent;
        EventStream<T> stream;
        int generation;

        public EventSubscription(SampleApp app, string name, Func<TwitchTask<EventStream<T>>> subscribe, Action<T> onEvent)
        {
            this.app = app;
            Name = name;
            this.subscribe = subscribe;
            this.onEvent = onEvent;
        }

        public async void Start()
        {
            if (State != SubscriptionState.Off || !Twitch.IsInitialized)
                return;

            var myGeneration = ++generation;
            SetState(SubscriptionState.Starting);
            EventStream<T> opened = null;
            try
            {
                opened = await subscribe();
                if (myGeneration != generation)
                    return; // stopped while subscribing

                stream = opened;
                SetState(SubscriptionState.Listening);
                app.Success($"Listening for {Name}.");

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
                    app.Error($"{Name}: {ex.Message}");
            }
            finally
            {
                if (myGeneration == generation)
                {
                    stream = null;
                    SetState(SubscriptionState.Off);
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
            SetState(SubscriptionState.Off);
            current?.Dispose();
            if (log)
                app.Info($"Stopped listening for {Name}.");
        }

        void SetState(SubscriptionState state)
        {
            State = state;
            StateChanged?.Invoke();
        }
    }
}
