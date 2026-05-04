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
                if (ext is not (".png" or ".wav" or ".fbx"))
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
        public ExportReport Export(string projectRoot, string exportName)
        {
            ProjectFolders.EnsureAll(projectRoot);

            var safeName = string.Concat((exportName ?? "Build").Where(ch => !Path.GetInvalidFileNameChars().Contains(ch))).Trim();
            if (string.IsNullOrWhiteSpace(safeName)) safeName = "Build";

            var outDir = Path.Combine(ProjectFolders.Exports(projectRoot), safeName);
            if (Directory.Exists(outDir))
                Directory.Delete(outDir, true);
            Directory.CreateDirectory(outDir);

            var report = new ExportReport { ExportDirectory = outDir };
            ValidateProjectReadiness(projectRoot, report);

            CopyIfExists(ProjectFolders.Scenes(projectRoot), Path.Combine(outDir, "Scenes"), report);
            CopyIfExists(ProjectFolders.AssetsProcessed(projectRoot), Path.Combine(outDir, "Assets"), report);
            CopyIfExists(Path.Combine(projectRoot, "mods"), Path.Combine(outDir, "mods"), report);

            string? exe = FindCandidateExe(projectRoot);
            if (exe is not null)
            {
                var exeOut = Path.Combine(outDir, Path.GetFileName(exe));
                File.Copy(exe, exeOut, true);
                report.ExecutablePath = exeOut;
                report.HasPlayableExe = true;
                report.PlatformArtifacts.WindowsExe = exeOut;
            }
            else
            {
                report.HasPlayableExe = false;
                report.Warnings.Add("No game executable found. Build native game target first, then export again.");
            }

            var runtimeDll = Directory.EnumerateFiles(projectRoot, "balmung*.dll", SearchOption.AllDirectories)
                .FirstOrDefault(p => !p.Contains("\\Exports\\", StringComparison.OrdinalIgnoreCase));
            if (runtimeDll is not null)
            {
                File.Copy(runtimeDll, Path.Combine(outDir, Path.GetFileName(runtimeDll)), true);
                report.CopiedFiles++;
            }

            CollectExistingPlatformArtifacts(projectRoot, outDir, report);
            GeneratePortablePlatformLaunchers(outDir, report);

            string exportBalpackPath = Path.Combine(outDir, safeName + ".balpack");
            File.WriteAllText(exportBalpackPath,
                JsonSerializer.Serialize(report, SceneJson.Options));
            report.BundlePath = exportBalpackPath;

            File.WriteAllText(Path.Combine(outDir, "export_manifest.json"),
                JsonSerializer.Serialize(report, SceneJson.Options));

            if (!report.HasPlayableExe)
            {
                File.WriteAllText(Path.Combine(outDir, "README-EXPORT.txt"),
                    "Export package created, but no playable .exe was found.\r\n" +
                    "Build balmung_game first, then run Export again.\r\n");
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

        private static void CollectExistingPlatformArtifacts(string projectRoot, string outDir, ExportReport report)
        {
            var app = Directory.EnumerateDirectories(projectRoot, "*.app", SearchOption.AllDirectories)
                .FirstOrDefault(p => !p.Contains("\\Exports\\", StringComparison.OrdinalIgnoreCase));
            if (app is not null)
            {
                string dst = Path.Combine(outDir, Path.GetFileName(app));
                DirectoryCopy(app, dst, true, report);
                report.PlatformArtifacts.MacApp = dst;
            }

            var deb = Directory.EnumerateFiles(projectRoot, "*.deb", SearchOption.AllDirectories)
                .FirstOrDefault(p => !p.Contains("\\Exports\\", StringComparison.OrdinalIgnoreCase));
            if (deb is not null)
            {
                string dst = Path.Combine(outDir, Path.GetFileName(deb));
                File.Copy(deb, dst, true);
                report.CopiedFiles++;
                report.PlatformArtifacts.LinuxDeb = dst;
            }

            var rpm = Directory.EnumerateFiles(projectRoot, "*.rpm", SearchOption.AllDirectories)
                .FirstOrDefault(p => !p.Contains("\\Exports\\", StringComparison.OrdinalIgnoreCase));
            if (rpm is not null)
            {
                string dst = Path.Combine(outDir, Path.GetFileName(rpm));
                File.Copy(rpm, dst, true);
                report.CopiedFiles++;
                report.PlatformArtifacts.LinuxRpm = dst;
            }

            var apk = Directory.EnumerateFiles(projectRoot, "*.apk", SearchOption.AllDirectories)
                .FirstOrDefault(p => !p.Contains("\\Exports\\", StringComparison.OrdinalIgnoreCase));
            if (apk is not null)
            {
                string dst = Path.Combine(outDir, Path.GetFileName(apk));
                File.Copy(apk, dst, true);
                report.CopiedFiles++;
                report.PlatformArtifacts.AndroidApk = dst;
            }

            var ipa = Directory.EnumerateFiles(projectRoot, "*.ipa", SearchOption.AllDirectories)
                .FirstOrDefault(p => !p.Contains("\\Exports\\", StringComparison.OrdinalIgnoreCase));
            if (ipa is not null)
            {
                string dst = Path.Combine(outDir, Path.GetFileName(ipa));
                File.Copy(ipa, dst, true);
                report.CopiedFiles++;
                report.PlatformArtifacts.IosIpa = dst;
            }
        }

        private static void GeneratePortablePlatformLaunchers(string outDir, ExportReport report)
        {
            // Export now always contains launch/build hints for each target platform.
            string scriptsDir = Path.Combine(outDir, "PlatformLaunchers");
            Directory.CreateDirectory(scriptsDir);
            File.WriteAllText(Path.Combine(scriptsDir, "run_windows.bat"), "@echo off\r\nif exist balmung_game.exe (\r\n  start \"\" balmung_game.exe\r\n) else (\r\n  echo balmung_game.exe not found in this export.\r\n)\r\n");
            File.WriteAllText(Path.Combine(scriptsDir, "run_linux.sh"), "#!/usr/bin/env bash\nset -e\nif [[ -f ./balmung_game ]]; then\n  chmod +x ./balmung_game\n  ./balmung_game\nelse\n  echo \"balmung_game not found in this export.\"\nfi\n");
            File.WriteAllText(Path.Combine(scriptsDir, "run_macos.sh"), "#!/usr/bin/env bash\nset -e\nif [[ -d ./Balmung.app ]]; then\n  open ./Balmung.app\nelse\n  echo \"Balmung.app not found in this export.\"\nfi\n");
            File.WriteAllText(Path.Combine(scriptsDir, "README-MOBILE.txt"),
                "Android: place signed .apk in export root (or produce from CI) and install with adb install.\r\n" +
                "iOS: place signed .ipa in export root; deployment requires Apple signing/notarization workflow.\r\n");
            report.CopiedFiles += 4;
        }
    }

    public enum AssetKind
    {
        Unknown,
        Texture,
        Audio,
        Model,
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
        public string? BundlePath { get; set; }
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
        public string? LinuxDeb { get; set; }
        public string? LinuxRpm { get; set; }
        public string? MacApp { get; set; }
        public string? AndroidApk { get; set; }
        public string? IosIpa { get; set; }
    }
}





