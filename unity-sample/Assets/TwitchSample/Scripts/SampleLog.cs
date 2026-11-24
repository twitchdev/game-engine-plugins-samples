using System;
using System.Collections.Generic;
using UnityEngine;

namespace TwitchSample
{
    public enum SampleLogLevel
    {
        Debug,
        Info,
        Success,
        Warning,
        Error,
    }

    public readonly struct SampleLogEntry
    {
        public readonly DateTime Time;
        public readonly SampleLogLevel Level;
        public readonly string Message;

        public SampleLogEntry(DateTime time, SampleLogLevel level, string message)
        {
            Time = time;
            Level = level;
            Message = message;
        }
    }

    /// <summary>
    /// The log shown at the bottom of the sample, mirrored to the Unity console.
    /// </summary>
    /// <remarks>
    /// The core library logs from background threads, so writes are locked. The UI never reads the
    /// live list; it takes a snapshot once per frame in Update so IMGUI's Layout and Repaint passes
    /// always see the same number of entries.
    /// </remarks>
    public static class SampleLog
    {
        const int MaxEntries = 300;

        static readonly object Lock = new object();
        static readonly List<SampleLogEntry> Entries = new List<SampleLogEntry>();
        static int Version;

        /// <summary>When true, the core library's Debug-level messages are shown as well.</summary>
        public static volatile bool ShowCoreDebugMessages;

        public static void Debug(string message) => Write(SampleLogLevel.Debug, message);
        public static void Info(string message) => Write(SampleLogLevel.Info, message);
        public static void Success(string message) => Write(SampleLogLevel.Success, message);
        public static void Warning(string message) => Write(SampleLogLevel.Warning, message);
        public static void Error(string message) => Write(SampleLogLevel.Error, message);

        public static void Write(SampleLogLevel level, string message)
        {
            lock (Lock)
            {
                Entries.Add(new SampleLogEntry(DateTime.Now, level, message));
                if (Entries.Count > MaxEntries)
                    Entries.RemoveRange(0, Entries.Count - MaxEntries);
                Version++;
            }

            // Debug.Log* is safe to call from any thread.
            switch (level)
            {
                case SampleLogLevel.Error:
                    UnityEngine.Debug.LogError("[TwitchSample] " + message);
                    break;
                case SampleLogLevel.Warning:
                    UnityEngine.Debug.LogWarning("[TwitchSample] " + message);
                    break;
                default:
                    UnityEngine.Debug.Log("[TwitchSample] " + message);
                    break;
            }
        }

        public static void Clear()
        {
            lock (Lock)
            {
                Entries.Clear();
                Version++;
            }
        }

        /// <summary>
        /// Refreshes <paramref name="snapshot"/> (newest entry first) if the log changed since
        /// <paramref name="lastVersion"/>. Returns true when the snapshot was updated.
        /// </summary>
        public static bool TryUpdateSnapshot(ref int lastVersion, ref SampleLogEntry[] snapshot)
        {
            lock (Lock)
            {
                if (lastVersion == Version)
                    return false;

                var copy = new SampleLogEntry[Entries.Count];
                for (int i = 0; i < copy.Length; i++)
                    copy[i] = Entries[Entries.Count - 1 - i];

                snapshot = copy;
                lastVersion = Version;
                return true;
            }
        }
    }
}
