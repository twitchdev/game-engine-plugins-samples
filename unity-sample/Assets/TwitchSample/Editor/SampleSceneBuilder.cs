using System.IO;
using System.Linq;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;

namespace TwitchSample.EditorTools
{
    /// <summary>
    /// Saves a scene containing the sample and adds it to the build settings.
    /// </summary>
    /// <remarks>
    /// You don't need this to try the sample in the Editor: pressing Play in any scene starts it.
    /// Use it when you want to make a standalone build.
    /// </remarks>
    public static class SampleSceneBuilder
    {
        public const string ScenePath = "Assets/TwitchSample/Scenes/TwitchSample.unity";

        [MenuItem("Twitch Sample/Create Sample Scene")]
        public static void CreateSampleScene()
        {
            if (!EditorSceneManager.SaveCurrentModifiedScenesIfUserWantsTo())
                return;

            Directory.CreateDirectory(Path.GetDirectoryName(ScenePath));

            var scene = EditorSceneManager.NewScene(NewSceneSetup.EmptyScene, NewSceneMode.Single);
            TwitchSampleApp.CreateCamera();
            new GameObject("Twitch Sample").AddComponent<TwitchSampleApp>();

            if (!EditorSceneManager.SaveScene(scene, ScenePath))
            {
                Debug.LogError("[TwitchSample] Could not save " + ScenePath);
                return;
            }

            // Put the sample scene first in the build settings so builds open it.
            var others = EditorBuildSettings.scenes.Where(s => s.path != ScenePath);
            EditorBuildSettings.scenes = new[] { new EditorBuildSettingsScene(ScenePath, true) }.Concat(others).ToArray();

            Debug.Log("[TwitchSample] Saved " + ScenePath + " and added it to the build settings.");
        }
    }
}
