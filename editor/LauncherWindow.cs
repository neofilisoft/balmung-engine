using System;
using System.IO;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Layout;
using Avalonia.Media;

namespace Balmung.Editor
{
    internal sealed class LauncherWindow : Window
    {
        public LauncherWindow()
        {
            Title = "Balmung";
            Width = 1100;
            Height = 720;
            MinWidth = 900;
            MinHeight = 620;
            Background = new SolidColorBrush(Color.Parse("#14161B"));

            string root = Directory.GetCurrentDirectory();
            string manifest = ProjectFolders.ProjectManifest(root);
            bool hasManifest = File.Exists(manifest);

            var heroTitle = new TextBlock
            {
                Text = "Balmung Engine",
                FontSize = 34,
                FontWeight = FontWeight.SemiBold,
                Foreground = new SolidColorBrush(Color.Parse("#F5F7FA")),
            };

            var heroBody = new TextBlock
            {
                Text = "Project shell and runtime hub for Balmung. Use this entry point to manage projects, bootstrap content workflows, and jump into the visual editor when needed.",
                FontSize = 15,
                TextWrapping = TextWrapping.Wrap,
                Foreground = new SolidColorBrush(Color.Parse("#B8C2D1")),
                MaxWidth = 680,
            };

            var primaryButton = CreateButton("Open Visual Editor", "#2563EB", (_, _) =>
            {
                var editor = new MainWindow();
                editor.Show();
                Close();
            });

            var ensureProjectButton = CreateButton("Prepare Project Folders", "#2F855A", (_, _) =>
            {
                ProjectFolders.EnsureAll(root);
                Close();
                new LauncherWindow().Show();
            });

            var projectCard = CreateCard(
                "Current Project",
                new TextBlock
                {
                    Text = root,
                    TextWrapping = TextWrapping.Wrap,
                    Foreground = new SolidColorBrush(Color.Parse("#E6ECF5")),
                    FontSize = 14,
                },
                new TextBlock
                {
                    Text = hasManifest ? "Project manifest detected and ready." : "No project manifest found yet. Balmung can scaffold one for you.",
                    Foreground = new SolidColorBrush(Color.Parse("#92A1B7")),
                    Margin = new Thickness(0, 8, 0, 0),
                });

            var runtimeCard = CreateCard(
                "Runtime Model",
                CreateBullet("`balmung.exe` is the main Balmung shell/runtime entry."),
                CreateBullet("`bal-editor.exe` opens the visual editor directly."),
                CreateBullet("Native runtime remains in `balmung.dll` and is loaded by the managed shell/editor."));

            var workflowCard = CreateCard(
                "Workflow",
                CreateBullet("Create/open a project from the shell."),
                CreateBullet("Jump into `bal-editor.exe` for scene, asset, and tooling workflows."),
                CreateBullet("Run native/runtime targets from the same repo as they mature toward full production runtime."));

            var body = new Grid
            {
                ColumnDefinitions = new ColumnDefinitions("1.4*,1*"),
                RowDefinitions = new RowDefinitions("Auto,Auto,*"),
                Margin = new Thickness(36, 32),
            };

            var buttons = new StackPanel
            {
                Orientation = Orientation.Horizontal,
                Spacing = 12,
                Margin = new Thickness(0, 18, 0, 0),
                Children =
                {
                    primaryButton,
                    ensureProjectButton,
                }
            };

            body.Children.Add(heroTitle);
            body.Children.Add(heroBody);
            Grid.SetRow(heroBody, 1);
            body.Children.Add(buttons);
            Grid.SetRow(buttons, 2);

            var rightPane = new StackPanel
            {
                Spacing = 16,
                Children = { projectCard, runtimeCard, workflowCard }
            };
            body.Children.Add(rightPane);
            Grid.SetColumn(rightPane, 1);
            Grid.SetRowSpan(rightPane, 3);

            Content = body;
        }

        private static Button CreateButton(string label, string background, EventHandler<Avalonia.Interactivity.RoutedEventArgs> onClick)
        {
            var button = new Button
            {
                Content = label,
                Padding = new Thickness(16, 10),
                MinWidth = 180,
                Background = new SolidColorBrush(Color.Parse(background)),
                Foreground = Brushes.White,
                BorderThickness = new Thickness(0),
                CornerRadius = new CornerRadius(8),
            };
            button.Click += onClick;
            return button;
        }

        private static Border CreateCard(string title, params Control[] body)
        {
            var stack = new StackPanel { Spacing = 8 };
            stack.Children.Add(new TextBlock
            {
                Text = title,
                FontSize = 15,
                FontWeight = FontWeight.SemiBold,
                Foreground = new SolidColorBrush(Color.Parse("#F2F5FA")),
            });
            foreach (var child in body)
            {
                stack.Children.Add(child);
            }

            return new Border
            {
                Background = new SolidColorBrush(Color.Parse("#1C212B")),
                BorderBrush = new SolidColorBrush(Color.Parse("#313847")),
                BorderThickness = new Thickness(1),
                CornerRadius = new CornerRadius(12),
                Padding = new Thickness(16, 14),
                Child = stack,
            };
        }

        private static TextBlock CreateBullet(string text) => new()
        {
            Text = "- " + text,
            TextWrapping = TextWrapping.Wrap,
            FontSize = 13,
            Foreground = new SolidColorBrush(Color.Parse("#A9B6C8")),
        };
    }
}
