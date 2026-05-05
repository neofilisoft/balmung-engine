using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.Json;

namespace Balmung.Editor
{
    public sealed class AssetTemplateService
    {
        private const string DefaultTemplateId = "gameplay-csharp";

        public AssetTemplateInstallReport InstallDefaultGameplayTemplate(string projectRoot)
            => Install(projectRoot, DefaultTemplateId);

        public AssetTemplateInstallReport Install(string projectRoot, string templateId)
        {
            ProjectFolders.EnsureAll(projectRoot);

            string templateRoot = FindTemplateRoot(projectRoot, templateId)
                ?? throw new DirectoryNotFoundException($"Asset template '{templateId}' was not found. Set BALMUNG_TEMPLATE_ROOT or ship templates/assets with the editor.");

            string manifestPath = Path.Combine(templateRoot, "template.json");
            AssetTemplateManifest manifest = File.Exists(manifestPath)
                ? JsonSerializer.Deserialize<AssetTemplateManifest>(File.ReadAllText(manifestPath), SceneJson.Options) ?? new AssetTemplateManifest()
                : new AssetTemplateManifest();

            string sourceDir = Path.Combine(templateRoot, string.IsNullOrWhiteSpace(manifest.SourceDirectory) ? "source" : manifest.SourceDirectory);
            if (!Directory.Exists(sourceDir))
                throw new DirectoryNotFoundException($"Template source folder is missing: {sourceDir}");

            string targetRel = string.IsNullOrWhiteSpace(manifest.TargetDirectory)
                ? Path.Combine("Assets", "Raw", "Templates", templateId)
                : manifest.TargetDirectory;
            string targetDir = Path.Combine(projectRoot, targetRel);
            Directory.CreateDirectory(targetDir);

            var report = new AssetTemplateInstallReport
            {
                TemplateId = string.IsNullOrWhiteSpace(manifest.Id) ? templateId : manifest.Id,
                DisplayName = string.IsNullOrWhiteSpace(manifest.DisplayName) ? templateId : manifest.DisplayName,
                TargetDirectory = targetDir,
            };

            foreach (var file in Directory.EnumerateFiles(sourceDir, "*.*", SearchOption.AllDirectories))
            {
                string rel = Path.GetRelativePath(sourceDir, file);
                string dst = Path.Combine(targetDir, rel);
                Directory.CreateDirectory(Path.GetDirectoryName(dst) ?? targetDir);
                bool changed = !File.Exists(dst) || File.GetLastWriteTimeUtc(dst) < File.GetLastWriteTimeUtc(file);
                File.Copy(file, dst, true);
                if (changed) report.InstalledFiles++;
                else report.UpdatedFiles++;
            }

            return report;
        }

        private static string? FindTemplateRoot(string projectRoot, string templateId)
        {
            foreach (string candidateRoot in EnumerateCandidateTemplateRoots(projectRoot))
            {
                string root = Path.Combine(candidateRoot, templateId);
                if (Directory.Exists(root))
                    return root;
            }

            return null;
        }

        private static IEnumerable<string> EnumerateCandidateTemplateRoots(string projectRoot)
        {
            string? env = Environment.GetEnvironmentVariable("BALMUNG_TEMPLATE_ROOT");
            if (!string.IsNullOrWhiteSpace(env))
                yield return env;

            yield return Path.Combine(AppContext.BaseDirectory, "templates", "assets");
            yield return Path.Combine(projectRoot, "templates", "assets");

            var dir = new DirectoryInfo(projectRoot);
            while (dir is not null)
            {
                yield return Path.Combine(dir.FullName, "templates", "assets");
                dir = dir.Parent;
            }
        }
    }

    public sealed class AssetTemplateManifest
    {
        public int SchemaVersion { get; set; } = 1;
        public string Id { get; set; } = "";
        public string DisplayName { get; set; } = "";
        public string Description { get; set; } = "";
        public string TargetDirectory { get; set; } = "";
        public string SourceDirectory { get; set; } = "source";
        public List<string> Files { get; set; } = new();
    }

    public sealed class AssetTemplateInstallReport
    {
        public string TemplateId { get; set; } = "";
        public string DisplayName { get; set; } = "";
        public string TargetDirectory { get; set; } = "";
        public int InstalledFiles { get; set; }
        public int UpdatedFiles { get; set; }
    }
}
