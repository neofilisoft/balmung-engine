using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;

namespace Balmung.Editor
{
    public static class ProjectFolders
    {
        public static string ProjectManifest(string root) => Path.Combine(root, "Project.balmung");
        public static string Scenes(string root) => Path.Combine(root, "Scenes");
        public static string AssetsRaw(string root) => Path.Combine(root, "Assets", "Raw");
        public static string AssetsProcessed(string root) => Path.Combine(root, "Assets", "Processed");
        public static string Exports(string root) => Path.Combine(root, "Exports");

        public static void EnsureAll(string root)
        {
            Directory.CreateDirectory(Scenes(root));
            Directory.CreateDirectory(AssetsRaw(root));
            Directory.CreateDirectory(AssetsProcessed(root));
            Directory.CreateDirectory(Exports(root));
            EnsureProjectManifest(root);
        }

        private static void EnsureProjectManifest(string root)
        {
            string manifestPath = ProjectManifest(root);
            if (File.Exists(manifestPath)) return;

            var manifest = new BalmungProjectFile
            {
                ProjectName = new DirectoryInfo(root).Name,
                StartupScene = "scene1.bmsc",
            };
            File.WriteAllText(manifestPath, JsonSerializer.Serialize(manifest, SceneJson.Options));
        }
    }

    public sealed class AssetPipelineService
    {
        public AssetPipelineReport Process(string projectRoot)
        {
            ProjectFolders.EnsureAll(projectRoot);

            var rawRoot = ProjectFolders.AssetsRaw(projectRoot);
            var outRoot = ProjectFolders.AssetsProcessed(projectRoot);
            var report = new AssetPipelineReport();
            var manifest = new AssetManifest();

            foreach (var file in Directory.EnumerateFiles(rawRoot, "*.*", SearchOption.AllDirectories))
            {
                var ext = Path.GetExtension(file).ToLowerInvariant();
                if (ext is not (".png" or ".wav" or ".fbx" or ".cs" or ".txt" or ".json" or ".tmj" or ".tsx" or ".aseprite"))
                    continue;

                try
                {
                    var kind = Classify(ext);
                    var rel = Path.GetRelativePath(rawRoot, file);
                    var subdir = kind switch
                    {
                        AssetKind.Texture => "textures",
                        AssetKind.Audio => "audio",
                        AssetKind.Model => "models",
                        AssetKind.Script => "scripts",
                        AssetKind.Document => "docs",
                        AssetKind.Tilemap => "tilemaps",
                        AssetKind.SpriteAnimation => "animations",
                        _ => "misc"
                    };

                    Validate(file, kind);

                    var outDir = Path.Combine(outRoot, subdir, Path.GetDirectoryName(rel) ?? "");
                    Directory.CreateDirectory(outDir);
                    var outPath = Path.Combine(outDir, Path.GetFileName(file));

                    bool changed = !File.Exists(outPath) || File.GetLastWriteTimeUtc(outPath) < File.GetLastWriteTimeUtc(file);
                    File.Copy(file, outPath, true);

                    var record = new AssetManifestItem
                    {
                        Source = Path.GetRelativePath(projectRoot, file),
                        Output = Path.GetRelativePath(projectRoot, outPath),
                        Kind = kind.ToString().ToLowerInvariant(),
                        Extension = ext,
                        ByteSize = new FileInfo(file).Length,
                        Sha256 = ComputeSha256(file),
                        ImportedUtc = DateTime.UtcNow,
                        Status = ext == ".fbx" ? "staged" : "ready",
                        Notes = ext == ".fbx" ? "FBX staged. Runtime conversion/importer backend still required for mesh optimization." : "",
                    };
                    manifest.Items.Add(record);
                    report.Items.Add(new AssetPipelineItemView(record.Source, record.Kind, record.Status, record.Output));
                    if (changed) report.ImportedCount++;
                    else report.SkippedCount++;
                }
                catch (Exception ex)
                {
                    report.ErrorCount++;
                    report.Errors.Add($"{Path.GetFileName(file)}: {ex.Message}");
                }
            }

            var jsonManifestPath = Path.Combine(outRoot, "asset_manifest.json");
            var balpackManifestPath = Path.Combine(outRoot, "assets.balpack");
            string manifestText = JsonSerializer.Serialize(manifest, SceneJson.Options);
            File.WriteAllText(jsonManifestPath, manifestText);
            File.WriteAllText(balpackManifestPath, manifestText);
            report.ManifestPath = balpackManifestPath;
            return report;
        }

        private static AssetKind Classify(string ext) => ext switch
        {
            ".png" => AssetKind.Texture,
            ".wav" => AssetKind.Audio,
            ".fbx" => AssetKind.Model,
            ".cs" => AssetKind.Script,
            ".txt" => AssetKind.Document,
            ".json" => AssetKind.Document,
            ".tmj" => AssetKind.Tilemap,
            ".tsx" => AssetKind.Tilemap,
            ".aseprite" => AssetKind.SpriteAnimation,
            _ => AssetKind.Unknown
        };

        private static void Validate(string path, AssetKind kind)
        {
            using var fs = File.OpenRead(path);
            switch (kind)
            {
                case AssetKind.Texture:
                    Span<byte> png = stackalloc byte[8];
                    if (fs.Read(png) != 8) throw new InvalidDataException("PNG file too short.");
                    byte[] sig = { 137, 80, 78, 71, 13, 10, 26, 10 };
                    for (int i = 0; i < 8; i++)
                        if (png[i] != sig[i]) throw new InvalidDataException("Invalid PNG signature.");
                    break;

                case AssetKind.Audio:
                    Span<byte> wav = stackalloc byte[12];
                    if (fs.Read(wav) != 12) throw new InvalidDataException("WAV file too short.");
                    if (Encoding.ASCII.GetString(wav.Slice(0, 4)) != "RIFF" ||
                        Encoding.ASCII.GetString(wav.Slice(8, 4)) != "WAVE")
                        throw new InvalidDataException("Invalid WAV header.");
                    break;

                case AssetKind.Model:
                    Span<byte> hdr = stackalloc byte[27];
                    int n = fs.Read(hdr);
                    if (n < 16) throw new InvalidDataException("FBX file too short.");
                    var prefix = Encoding.ASCII.GetString(hdr.Slice(0, n).ToArray());
                    if (!prefix.Contains("FBX", StringComparison.OrdinalIgnoreCase) &&
                        !prefix.StartsWith("Kaydara FBX Binary", StringComparison.Ordinal))
                        throw new InvalidDataException("Unrecognized FBX header.");
                    break;

                case AssetKind.Script:
                case AssetKind.Document:
                case AssetKind.Tilemap:
                case AssetKind.SpriteAnimation:
                    if (fs.Length == 0) throw new InvalidDataException("Text asset is empty.");
                    break;
            }
        }

        private static string ComputeSha256(string path)
        {
            using var sha = SHA256.Create();
            using var fs = File.OpenRead(path);
            return Convert.ToHexString(sha.ComputeHash(fs));
        }
    }

    public sealed class StandaloneExportService
    {
        public ExportReport Export(string projectRoot, string exportName, string targetPlatform)
        {
            ProjectFolders.EnsureAll(projectRoot);
            string platform = ExportTargetPlatforms.Normalize(targetPlatform);

            var safeName = string.Concat((exportName ?? "Build").Where(ch => !Path.GetInvalidFileNameChars().Contains(ch))).Trim();
            if (string.IsNullOrWhiteSpace(safeName)) safeName = "Build";

            var outDir = Path.Combine(ProjectFolders.Exports(projectRoot), safeName);
            if (Directory.Exists(outDir))
                Directory.Delete(outDir, true);
            Directory.CreateDirectory(outDir);

            var report = new ExportReport { ExportDirectory = outDir, TargetPlatform = platform };
            ValidateProjectReadiness(projectRoot, report);

            CopyIfExists(ProjectFolders.Scenes(projectRoot), Path.Combine(outDir, "Scenes"), report);
            CopyIfExists(ProjectFolders.AssetsProcessed(projectRoot), Path.Combine(outDir, "Assets"), report);
            CopyIfExists(Path.Combine(projectRoot, "mods"), Path.Combine(outDir, "mods"), report);

            CopyTargetPlatformArtifact(projectRoot, outDir, platform, report);

            var runtimeDll = Directory.EnumerateFiles(projectRoot, "balmung*.dll", SearchOption.AllDirectories)
                .FirstOrDefault(p => !p.Contains("\\Exports\\", StringComparison.OrdinalIgnoreCase));
            if (runtimeDll is not null)
            {
                File.Copy(runtimeDll, Path.Combine(outDir, Path.GetFileName(runtimeDll)), true);
                report.CopiedFiles++;
            }

            GenerateTargetPlatformHandoff(outDir, platform, report);

            string exportBalpackPath = Path.Combine(outDir, safeName + ".balpack");
            File.WriteAllText(exportBalpackPath,
                JsonSerializer.Serialize(report, SceneJson.Options));
            report.BundlePath = exportBalpackPath;

            File.WriteAllText(Path.Combine(outDir, "export_manifest.json"),
                JsonSerializer.Serialize(report, SceneJson.Options));

            if (!report.HasTargetArtifact)
            {
                File.WriteAllText(Path.Combine(outDir, "README-EXPORT.txt"),
                    $"Export package created for {platform}, but no platform artifact was found yet.\r\n" +
                    ExportTargetPlatforms.BuildHint(platform) + "\r\n");
            }

            return report;
        }

        private static void ValidateProjectReadiness(string projectRoot, ExportReport report)
        {
            string scenes = ProjectFolders.Scenes(projectRoot);
            string processed = ProjectFolders.AssetsProcessed(projectRoot);
            bool hasBalmungScenes = Directory.Exists(scenes) && Directory.EnumerateFiles(scenes, "*.bmsc", SearchOption.TopDirectoryOnly).Any();
            bool hasLegacyBalmungScenes = Directory.Exists(scenes) && Directory.EnumerateFiles(scenes, "*.balmung", SearchOption.TopDirectoryOnly).Any();
            bool hasLegacyJsonScenes = Directory.Exists(scenes) && Directory.EnumerateFiles(scenes, "*.json", SearchOption.TopDirectoryOnly).Any();
            if (!hasBalmungScenes && !hasLegacyBalmungScenes && !hasLegacyJsonScenes)
                report.Warnings.Add("No scene files found in Scenes/. Create and save at least one .bmsc scene.");
            if (!File.Exists(Path.Combine(processed, "assets.balpack")) && !File.Exists(Path.Combine(processed, "asset_manifest.json")))
                report.Warnings.Add("assets.balpack not found. Run Process Assets before export.");
            if (!Directory.Exists(Path.Combine(projectRoot, "mods")))
                report.Warnings.Add("mods/ folder not found. Lua mods are optional, but package is lean.");
            if (!File.Exists(ProjectFolders.ProjectManifest(projectRoot)))
                report.Warnings.Add("Project.balmung not found at project root.");
        }

        private static void CopyIfExists(string src, string dst, ExportReport report)
        {
            if (!Directory.Exists(src)) return;
            DirectoryCopy(src, dst, true, report);
        }

        private static void DirectoryCopy(string src, string dst, bool recursive, ExportReport report)
        {
            var dir = new DirectoryInfo(src);
            if (!dir.Exists) return;
            Directory.CreateDirectory(dst);
            foreach (var file in dir.GetFiles())
            {
                file.CopyTo(Path.Combine(dst, file.Name), true);
                report.CopiedFiles++;
            }

            if (!recursive) return;
            foreach (var sub in dir.GetDirectories())
                DirectoryCopy(sub.FullName, Path.Combine(dst, sub.Name), true, report);
        }

        private static string? FindCandidateExe(string projectRoot)
        {
            string[] preferred =
            {
                Path.Combine(projectRoot, "build", "balmung_game.exe"),
                Path.Combine(projectRoot, "build-native", "balmung_game.exe"),
                Path.Combine(projectRoot, "bin", "balmung_game.exe"),
            };
            foreach (var p in preferred)
                if (File.Exists(p)) return p;

            return Directory.EnumerateFiles(projectRoot, "*.exe", SearchOption.AllDirectories)
                .Where(p => p.Contains("balmung", StringComparison.OrdinalIgnoreCase))
                .Where(p => p.Contains("game", StringComparison.OrdinalIgnoreCase))
                .Where(p => !p.Contains("\\Exports\\", StringComparison.OrdinalIgnoreCase))
                .FirstOrDefault();
        }

        private static void CopyTargetPlatformArtifact(string projectRoot, string outDir, string platform, ExportReport report)
        {
            switch (platform)
            {
                case ExportTargetPlatforms.Windows:
                    CopyWindowsArtifact(projectRoot, outDir, report);
                    break;
                case ExportTargetPlatforms.Linux:
                    CopyFirstFileArtifact(projectRoot, outDir, report, p => report.PlatformArtifacts.LinuxPackage = p, "balmung_game", "*.AppImage", "*.deb", "*.rpm");
                    break;
                case ExportTargetPlatforms.MacOS:
                    CopyMacArtifact(projectRoot, outDir, report);
                    break;
                case ExportTargetPlatforms.Android:
                    CopyFirstFileArtifact(projectRoot, outDir, report, p => report.PlatformArtifacts.AndroidApk = p, "*.apk");
                    break;
                case ExportTargetPlatforms.IOS:
                    CopyFirstFileArtifact(projectRoot, outDir, report, p => report.PlatformArtifacts.IosIpa = p, "*.ipa");
                    break;
                case ExportTargetPlatforms.Web:
                    CopyWebArtifact(projectRoot, outDir, report);
                    break;
            }
        }

        private static void CopyWindowsArtifact(string projectRoot, string outDir, ExportReport report)
        {
            string? exe = FindCandidateExe(projectRoot);
            if (exe is null)
            {
                report.Warnings.Add("No Windows game executable found. Build native game target first, then export again.");
                return;
            }

            var exeOut = Path.Combine(outDir, Path.GetFileName(exe));
            File.Copy(exe, exeOut, true);
            report.ExecutablePath = exeOut;
            report.HasPlayableExe = true;
            report.HasTargetArtifact = true;
            report.PlatformArtifacts.WindowsExe = exeOut;
        }

        private static void CopyFirstFileArtifact(string projectRoot, string outDir, ExportReport report, Action<string> assign, params string[] patterns)
        {
            string? artifact = patterns
                .SelectMany(pattern => Directory.EnumerateFiles(projectRoot, pattern, SearchOption.AllDirectories))
                .Where(IsExportCandidatePath)
                .FirstOrDefault();

            if (artifact is null)
            {
                report.Warnings.Add($"No {report.TargetPlatform} artifact found. {ExportTargetPlatforms.BuildHint(report.TargetPlatform)}");
                return;
            }

            string dst = Path.Combine(outDir, Path.GetFileName(artifact));
            File.Copy(artifact, dst, true);
            report.CopiedFiles++;
            report.HasTargetArtifact = true;
            assign(dst);
        }

        private static void CopyMacArtifact(string projectRoot, string outDir, ExportReport report)
        {
            var app = Directory.EnumerateDirectories(projectRoot, "*.app", SearchOption.AllDirectories)
                .FirstOrDefault(IsExportCandidatePath);
            if (app is null)
            {
                report.Warnings.Add($"No macOS .app bundle found. {ExportTargetPlatforms.BuildHint(report.TargetPlatform)}");
                return;
            }

            string dst = Path.Combine(outDir, Path.GetFileName(app));
            DirectoryCopy(app, dst, true, report);
            report.HasTargetArtifact = true;
            report.PlatformArtifacts.MacApp = dst;
        }

        private static void CopyWebArtifact(string projectRoot, string outDir, ExportReport report)
        {
            string? index = Directory.EnumerateFiles(projectRoot, "index.html", SearchOption.AllDirectories)
                .Where(IsExportCandidatePath)
                .FirstOrDefault(p => Directory.EnumerateFiles(Path.GetDirectoryName(p) ?? projectRoot, "*.wasm", SearchOption.AllDirectories).Any());

            if (index is null)
            {
                report.Warnings.Add($"No Web index.html + .wasm bundle found. {ExportTargetPlatforms.BuildHint(report.TargetPlatform)}");
                return;
            }

            string srcDir = Path.GetDirectoryName(index) ?? projectRoot;
            string dst = Path.Combine(outDir, "Web");
            DirectoryCopy(srcDir, dst, true, report);
            report.HasTargetArtifact = true;
            report.PlatformArtifacts.WebBundle = dst;
        }

        private static bool IsExportCandidatePath(string path)
            => !path.Contains($"{Path.DirectorySeparatorChar}Exports{Path.DirectorySeparatorChar}", StringComparison.OrdinalIgnoreCase)
               && !path.Contains($"{Path.DirectorySeparatorChar}obj{Path.DirectorySeparatorChar}", StringComparison.OrdinalIgnoreCase);

        private static void GenerateTargetPlatformHandoff(string outDir, string platform, ExportReport report)
        {
            string scriptsDir = Path.Combine(outDir, "PlatformLaunchers");
            Directory.CreateDirectory(scriptsDir);
            File.WriteAllText(Path.Combine(scriptsDir, "README-TARGET.txt"),
                $"Target platform: {platform}\r\n" +
                ExportTargetPlatforms.BuildHint(platform) + "\r\n");

            if (platform == ExportTargetPlatforms.Windows)
                File.WriteAllText(Path.Combine(scriptsDir, "run_windows.bat"), "@echo off\r\nif exist balmung_game.exe (\r\n  start \"\" balmung_game.exe\r\n) else (\r\n  echo balmung_game.exe not found in this export.\r\n)\r\n");
            else if (platform == ExportTargetPlatforms.Linux)
                File.WriteAllText(Path.Combine(scriptsDir, "run_linux.sh"), "#!/usr/bin/env bash\nset -e\nif [[ -f ./balmung_game ]]; then\n  chmod +x ./balmung_game\n  ./balmung_game\nelse\n  echo \"balmung_game not found in this export.\"\nfi\n");
            else if (platform == ExportTargetPlatforms.MacOS)
                File.WriteAllText(Path.Combine(scriptsDir, "run_macos.sh"), "#!/usr/bin/env bash\nset -e\nif [[ -d ./Balmung.app ]]; then\n  open ./Balmung.app\nelse\n  echo \"Balmung.app not found in this export.\"\nfi\n");

            report.CopiedFiles++;
        }
    }

    public enum AssetKind
    {
        Unknown,
        Texture,
        Audio,
        Model,
        Script,
        Document,
        Tilemap,
        SpriteAnimation,
    }

    public sealed class AssetPipelineReport
    {
        public int ImportedCount { get; set; }
        public int SkippedCount { get; set; }
        public int ErrorCount { get; set; }
        public string ManifestPath { get; set; } = "";
        public List<string> Errors { get; } = new();
        public List<AssetPipelineItemView> Items { get; } = new();
    }

    public sealed record AssetPipelineItemView(string Source, string Kind, string Status, string Output);

    public sealed class AssetManifest
    {
        public List<AssetManifestItem> Items { get; set; } = new();
    }

    public sealed class AssetManifestItem
    {
        public string Source { get; set; } = "";
        public string Output { get; set; } = "";
        public string Kind { get; set; } = "";
        public string Extension { get; set; } = "";
        public long ByteSize { get; set; }
        public string Sha256 { get; set; } = "";
        public DateTime ImportedUtc { get; set; }
        public string Status { get; set; } = "";
        public string Notes { get; set; } = "";
    }

    public sealed class ExportReport
    {
        public string ExportDirectory { get; set; } = "";
        public string TargetPlatform { get; set; } = ExportTargetPlatforms.Windows;
        public string? BundlePath { get; set; }
        public bool HasTargetArtifact { get; set; }
        public bool HasPlayableExe { get; set; }
        public string? ExecutablePath { get; set; }
        public int CopiedFiles { get; set; }
        public List<string> Warnings { get; set; } = new();
        public PlatformArtifactReport PlatformArtifacts { get; set; } = new();
    }

    public sealed class BalmungProjectFile
    {
        public string Format { get; set; } = "BalmungProject";
        public string Kind { get; set; } = "project";
        public string Engine { get; set; } = "Balmung Engine";
        public int SchemaVersion { get; set; } = 1;
        public string ProjectName { get; set; } = "Balmung Project";
        public string StartupScene { get; set; } = "scene1.bmsc";
        public string ScenesDirectory { get; set; } = "Scenes";
        public string RawAssetsDirectory { get; set; } = Path.Combine("Assets", "Raw");
        public string ProcessedAssetsDirectory { get; set; } = Path.Combine("Assets", "Processed");
        public string AssetPack { get; set; } = Path.Combine("Assets", "Processed", "assets.balpack");
        public string ExportsDirectory { get; set; } = "Exports";
    }

    public sealed class PlatformArtifactReport
    {
        public string? WindowsExe { get; set; }
        public string? LinuxPackage { get; set; }
        public string? MacApp { get; set; }
        public string? AndroidApk { get; set; }
        public string? IosIpa { get; set; }
        public string? WebBundle { get; set; }
    }

    public static class ExportTargetPlatforms
    {
        public const string Android = "Android";
        public const string IOS = "iOS";
        public const string Windows = "Windows";
        public const string Linux = "Linux";
        public const string MacOS = "macOS";
        public const string Web = "Web";

        public static IReadOnlyList<string> All { get; } = new[]
        {
            Windows,
            Linux,
            MacOS,
            Android,
            IOS,
            Web,
        };

        public static string Normalize(string? platform)
        {
            string value = (platform ?? Windows).Trim();
            return value.ToLowerInvariant() switch
            {
                "android" => Android,
                "ios" => IOS,
                "iphone" => IOS,
                "windows" => Windows,
                "win" => Windows,
                "linux" => Linux,
                "macos" => MacOS,
                "mac" => MacOS,
                "osx" => MacOS,
                "web" => Web,
                "wasm" => Web,
                _ => Windows,
            };
        }

        public static string BuildHint(string platform) => Normalize(platform) switch
        {
            Android => "Android export expects a signed .apk from the Android build/signing pipeline.",
            IOS => "iOS export expects a signed .ipa from an Apple signing and deployment pipeline.",
            Linux => "Linux export expects balmung_game, an AppImage, .deb, or .rpm produced by the native Linux build.",
            MacOS => "macOS export expects a Balmung .app bundle produced by a macOS build/signing pipeline.",
            Web => "Web export expects index.html plus .wasm output from the WebAssembly build pipeline.",
            _ => "Windows export expects balmung_game.exe from the native game build.",
        };
    }
}





