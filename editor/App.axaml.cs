using Avalonia;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Markup.Xaml;
using System;
using System.IO;
using System.Linq;

namespace Balmung.Editor
{
    public partial class App : Application
    {
        public override void Initialize() => AvaloniaXamlLoader.Load(this);

        public override void OnFrameworkInitializationCompleted()
        {
            if (ApplicationLifetime is IClassicDesktopStyleApplicationLifetime desktop)
            {
                var args = desktop.Args ?? Array.Empty<string>();
                var exeName = Path.GetFileNameWithoutExtension(Environment.ProcessPath ?? "bal-editor");
                var openEditor = args.Any(a => string.Equals(a, "--editor", StringComparison.OrdinalIgnoreCase)) ||
                                 string.Equals(exeName, "bal-editor", StringComparison.OrdinalIgnoreCase);
                desktop.MainWindow = openEditor ? new MainWindow() : new LauncherWindow();
            }
            base.OnFrameworkInitializationCompleted();
        }
    }
}


