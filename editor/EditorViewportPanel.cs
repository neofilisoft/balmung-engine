using System;
using System.Numerics;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Interactivity;
using Avalonia.Media;
using Avalonia.Threading;
using Balmung.Bridge;
using AvaloniaVector = Avalonia.Vector;

namespace Balmung.Editor
{
    public sealed class EditorViewportPanel : UserControl
    {
        public static readonly StyledProperty<long> EngineHandleProperty = AvaloniaProperty.Register<EditorViewportPanel, long>(nameof(EngineHandle));
        public static readonly StyledProperty<string?> WorldNameProperty = AvaloniaProperty.Register<EditorViewportPanel, string?>(nameof(WorldName), "world1");
        public static readonly StyledProperty<string?> SeedTextProperty = AvaloniaProperty.Register<EditorViewportPanel, string?>(nameof(SeedText), "0");
        public static readonly StyledProperty<string?> ChunkCountTextProperty = AvaloniaProperty.Register<EditorViewportPanel, string?>(nameof(ChunkCountText), "0");
        public static readonly StyledProperty<string?> EditorModeProperty = AvaloniaProperty.Register<EditorViewportPanel, string?>(nameof(EditorMode), "Scene");
        public static readonly StyledProperty<string?> ToolLabelProperty = AvaloniaProperty.Register<EditorViewportPanel, string?>(nameof(ToolLabel), "Select");
        public static readonly StyledProperty<string?> StatsTextProperty = AvaloniaProperty.Register<EditorViewportPanel, string?>(nameof(StatsText), "");
        public static readonly StyledProperty<string?> StatusTextProperty = AvaloniaProperty.Register<EditorViewportPanel, string?>(nameof(StatusText), "Ready");

        private readonly ViewportSurface _surface;
        private readonly TextBlock _header;
        private readonly TextBlock _camera;
        private readonly TextBlock _footer;
        private readonly Button _translate;
        private readonly Button _rotate;
        private readonly Button _scale;
        private readonly Button _local;
        private readonly Button _world;
        private readonly Button _perspective;
        private readonly Button _top;
        private readonly Button _front;
        private readonly Button _iso;
        private readonly DispatcherTimer _timer;

        public long EngineHandle { get => GetValue(EngineHandleProperty); set => SetValue(EngineHandleProperty, value); }
        public string? WorldName { get => GetValue(WorldNameProperty); set => SetValue(WorldNameProperty, value); }
        public string? SeedText { get => GetValue(SeedTextProperty); set => SetValue(SeedTextProperty, value); }
        public string? ChunkCountText { get => GetValue(ChunkCountTextProperty); set => SetValue(ChunkCountTextProperty, value); }
        public string? EditorMode { get => GetValue(EditorModeProperty); set => SetValue(EditorModeProperty, value); }
        public string? ToolLabel { get => GetValue(ToolLabelProperty); set => SetValue(ToolLabelProperty, value); }
        public string? StatsText { get => GetValue(StatsTextProperty); set => SetValue(StatsTextProperty, value); }
        public string? StatusText { get => GetValue(StatusTextProperty); set => SetValue(StatusTextProperty, value); }

        public EditorViewportPanel()
        {
            _surface = new ViewportSurface();
            _surface.ChromeChanged += (_, _) => RefreshChrome();

            _header = Label(13, Brushes.White);
            _camera = Label(12, new SolidColorBrush(Color.FromRgb(145, 201, 255)));
            _footer = Label(12, new SolidColorBrush(Color.FromRgb(190, 190, 190)));
            _translate = ToolButton("Translate", (_, _) => SetOperation(ViewportOp.Translate));
            _rotate = ToolButton("Rotate", (_, _) => SetOperation(ViewportOp.Rotate));
            _scale = ToolButton("Scale", (_, _) => SetOperation(ViewportOp.Scale));
            _local = ToolButton("Local", (_, _) => SetSpace(ViewportSpace.Local));
            _world = ToolButton("World", (_, _) => SetSpace(ViewportSpace.World));
            _perspective = ToolButton("Perspective", (_, _) => SetView(ViewPreset.Perspective));
            _top = ToolButton("Top", (_, _) => SetView(ViewPreset.Top));
            _front = ToolButton("Front", (_, _) => SetView(ViewPreset.Front));
            _iso = ToolButton("Iso", (_, _) => SetView(ViewPreset.Isometric));

            var toolbar = new Border
            {
                Background = Brush("#DE1A1A1A"),
                Padding = new Thickness(10, 8),
                Child = new StackPanel
                {
                    Orientation = Avalonia.Layout.Orientation.Horizontal,
                    Spacing = 6,
                    Children = { _translate, _rotate, _scale, Gap(), _local, _world, Gap(), _perspective, _top, _front, _iso }
                }
            };

            var chrome = new Grid { RowDefinitions = new RowDefinitions("Auto,*,Auto"), IsHitTestVisible = false };
            chrome.Children.Add(toolbar);
            Grid.SetRow(toolbar, 0);
            toolbar.IsHitTestVisible = true;

            var overlay = new Grid
            {
                Margin = new Thickness(14, 12),
                ColumnDefinitions = new ColumnDefinitions("Auto,*,Auto"),
                RowDefinitions = new RowDefinitions("Auto,*,Auto")
            };
            var leftCard = Card(_header, Label(12, new SolidColorBrush(Color.FromRgb(170, 220, 170)), "RMB Orbit | MMB Pan | Wheel Dolly | T/E/R | 1/3/7/9"));
            var rightCard = Card(_camera);
            overlay.Children.Add(leftCard);
            overlay.Children.Add(rightCard);
            Grid.SetColumn(rightCard, 2);
            chrome.Children.Add(overlay);
            Grid.SetRow(overlay, 1);

            var footerBorder = new Border
            {
                Background = Brush("#E0121212"),
                Padding = new Thickness(10, 8),
                Child = _footer
            };
            chrome.Children.Add(footerBorder);
            Grid.SetRow(footerBorder, 2);

            var root = new Grid();
            root.Children.Add(_surface);
            root.Children.Add(chrome);
            Content = root;

            _timer = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(120) };
            _timer.Tick += (_, _) => RefreshChrome();
            _timer.Start();

            SetOperation(ViewportOp.Translate);
            SetSpace(ViewportSpace.World);
            SetView(ViewPreset.Isometric);
            RefreshChrome();
        }

        protected override void OnDetachedFromVisualTree(VisualTreeAttachmentEventArgs e)
        {
            _timer.Stop();
            base.OnDetachedFromVisualTree(e);
        }

        protected override void OnPropertyChanged(AvaloniaPropertyChangedEventArgs change)
        {
            base.OnPropertyChanged(change);
            if (change.Property == EngineHandleProperty)
            {
                _surface.EngineHandle = change.GetNewValue<long>();
                _surface.InvalidateVisual();
            }
            else if (change.Property == WorldNameProperty ||
                     change.Property == SeedTextProperty ||
                     change.Property == ChunkCountTextProperty ||
                     change.Property == EditorModeProperty ||
                     change.Property == ToolLabelProperty ||
                     change.Property == StatsTextProperty ||
                     change.Property == StatusTextProperty)
            {
                RefreshChrome();
            }
        }

        private void SetOperation(ViewportOp op)
        {
            _surface.Operation = op;
            RefreshButtons();
            _surface.InvalidateVisual();
        }

        private void SetSpace(ViewportSpace space)
        {
            _surface.Space = space;
            RefreshButtons();
            _surface.InvalidateVisual();
        }

        private void SetView(ViewPreset preset)
        {
            _surface.SetPresetView(preset);
            RefreshButtons();
            RefreshChrome();
            _surface.InvalidateVisual();
        }

        private void RefreshChrome()
        {
            _header.Text = $"3D Scene View | {WorldName ?? "world1"} | Seed {SeedText ?? "0"} | Chunks {ChunkCountText ?? "0"} | {EditorMode ?? "Scene"}/{ToolLabel ?? "Select"}";
            _camera.Text = _surface.CameraSummary;
            _footer.Text = string.IsNullOrWhiteSpace(StatusText)
                ? (StatsText ?? "Viewport ready")
                : $"{StatusText}    {(string.IsNullOrWhiteSpace(StatsText) ? string.Empty : "|  " + StatsText)}";
        }

        private void RefreshButtons()
        {
            Mark(_translate, _surface.Operation == ViewportOp.Translate);
            Mark(_rotate, _surface.Operation == ViewportOp.Rotate);
            Mark(_scale, _surface.Operation == ViewportOp.Scale);
            Mark(_local, _surface.Space == ViewportSpace.Local);
            Mark(_world, _surface.Space == ViewportSpace.World);
            Mark(_perspective, _surface.ViewPreset == ViewPreset.Perspective);
            Mark(_top, _surface.ViewPreset == ViewPreset.Top);
            Mark(_front, _surface.ViewPreset == ViewPreset.Front);
            Mark(_iso, _surface.ViewPreset == ViewPreset.Isometric);
        }

        private static void Mark(Button button, bool active)
        {
            button.Background = active ? Brush("#2563EB") : Brush("#363636");
            button.BorderBrush = active ? Brush("#7DB2FF") : Brush("#4A4A4A");
        }

        private static Border Card(params Control[] children)
        {
            var panel = new StackPanel { Spacing = 4 };
            foreach (var c in children)
            {
                panel.Children.Add(c);
            }
            return new Border
            {
                Background = Brush("#C80E0E0E"),
                BorderBrush = Brush("#64FFFFFF"),
                BorderThickness = new Thickness(1),
                CornerRadius = new CornerRadius(10),
                Padding = new Thickness(10, 8),
                Child = panel
            };
        }

        private static TextBlock Label(double size, IBrush brush, string? text = null) =>
            new() { Text = text ?? string.Empty, FontSize = size, Foreground = brush, TextWrapping = TextWrapping.Wrap };

        private static Border Gap() => new() { Width = 8, Opacity = 0 };
        private static SolidColorBrush Brush(string hex) => new(Color.Parse(hex));

        private static Button ToolButton(string label, EventHandler<RoutedEventArgs> onClick)
        {
            var b = new Button
            {
                Content = label,
                MinWidth = 74,
                Padding = new Thickness(10, 4),
                Background = Brush("#363636"),
                Foreground = Brushes.White,
                BorderBrush = Brush("#4A4A4A"),
                BorderThickness = new Thickness(1),
                CornerRadius = new CornerRadius(6)
            };
            b.Click += onClick;
            return b;
        }
    }

    internal enum ViewportOp { Translate, Rotate, Scale }
    internal enum ViewportSpace { Local, World }
    internal enum ViewPreset { Perspective, Top, Front, Isometric }
    internal enum GizmoAxis { None, X, Y, Z }

    internal sealed class ViewportSurface : Control
    {
        public static readonly StyledProperty<long> EngineHandleProperty = AvaloniaProperty.Register<ViewportSurface, long>(nameof(EngineHandle));

        private readonly DispatcherTimer _timer;
        private Point? _lastPointer;
        private bool _orbit;
        private bool _pan;
        private bool _draggingGizmo;
        private GizmoAxis _activeAxis = GizmoAxis.None;
        private Point _dragStartScreen;
        private Vector3 _dragStartPosition;
        private Vector3 _dragStartScale;
        private Vector3 _dragStartRotationDeg;
        private uint _textureId;
        private RenderStats _stats;
        private int _chunks;
        private uint _seed;

        private Vector3 _focus = new(0.0f, 1.5f, 0.0f);
        private double _yawDeg = -35.0;
        private double _pitchDeg = 28.0;
        private double _distance = 12.0;
        private Vector2 _panOffset = Vector2.Zero;
        private bool _orthographic;

        private Vector3 _selectionPosition = new(0.0f, 1.6f, 0.0f);
        private Vector3 _selectionRotationDeg = new(0.0f, 18.0f, 0.0f);
        private Vector3 _selectionScale = new(2.4f, 1.6f, 2.0f);

        public event EventHandler? ChromeChanged;

        public long EngineHandle { get => GetValue(EngineHandleProperty); set => SetValue(EngineHandleProperty, value); }
        public ViewportOp Operation { get; set; } = ViewportOp.Translate;
        public ViewportSpace Space { get; set; } = ViewportSpace.World;
        public ViewPreset ViewPreset { get; private set; } = ViewPreset.Isometric;

        public string CameraSummary =>
            $"{(ViewPreset == ViewPreset.Perspective ? "Perspective" : ViewPreset)} | {Operation} | {Space} | yaw {_yawDeg:F0}° pitch {_pitchDeg:F0}° dolly {_distance:F1}";

        public ViewportSurface()
        {
            Focusable = true;
            ClipToBounds = true;
            PointerPressed += OnPressed;
            PointerMoved += OnMoved;
            PointerReleased += OnReleased;
            PointerWheelChanged += OnWheel;
            KeyDown += OnKeyDown;
            SizeChanged += (_, e) => ResizeNative((int)e.NewSize.Width, (int)e.NewSize.Height);

            _timer = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(80) };
            _timer.Tick += (_, _) => { Poll(); InvalidateVisual(); };
            _timer.Start();
        }

        protected override void OnDetachedFromVisualTree(VisualTreeAttachmentEventArgs e)
        {
            _timer.Stop();
            base.OnDetachedFromVisualTree(e);
        }

        protected override void OnPropertyChanged(AvaloniaPropertyChangedEventArgs change)
        {
            base.OnPropertyChanged(change);
            if (change.Property == EngineHandleProperty)
            {
                Poll();
                InvalidateVisual();
            }
        }

        public void SetPresetView(ViewPreset preset)
        {
            ViewPreset = preset;
            _orthographic = preset is ViewPreset.Top or ViewPreset.Front;
            switch (preset)
            {
            case ViewPreset.Perspective:
                _yawDeg = -24.0;
                _pitchDeg = 18.0;
                _distance = 11.0;
                break;
            case ViewPreset.Top:
                _yawDeg = 0.0;
                _pitchDeg = 89.0;
                _distance = 14.0;
                break;
            case ViewPreset.Front:
                _yawDeg = 0.0;
                _pitchDeg = 0.0;
                _distance = 11.0;
                break;
            default:
                _yawDeg = -35.0;
                _pitchDeg = 28.0;
                _distance = 12.0;
                break;
            }
            ChromeChanged?.Invoke(this, EventArgs.Empty);
        }

        public override void Render(DrawingContext context)
        {
            base.Render(context);
            var rect = new Rect(Bounds.Size);
            context.FillRectangle(CreateBackgroundBrush(), rect);

            if (Bounds.Width < 32 || Bounds.Height < 32)
            {
                return;
            }

            var view = CreateViewMatrix();
            var projection = CreateProjectionMatrix();
            var viewProjection = view * projection;

            DrawGrid(context, rect, viewProjection);
            DrawOriginAxes(context, rect, viewProjection);
            DrawSelectionCube(context, rect, viewProjection);
            DrawGizmo(context, rect, viewProjection);
            DrawViewCube(context, rect);

            DrawLabel(context, CameraSummary, new Point(rect.Right - 255, rect.Top + 14), 11, new SolidColorBrush(Color.FromArgb(190, 255, 255, 255)));
            var fps = _stats.FrameMs > 0.001f ? 1000f / _stats.FrameMs : 0f;
            DrawLabel(context, $"Texture {_textureId} | Chunks {_chunks} | Draw {_stats.DrawCalls} | {_stats.FrameMs:F2} ms / {fps:F0} FPS", new Point(18, rect.Bottom - 28), 11, Brushes.White);
        }

        private void DrawGrid(DrawingContext context, Rect rect, Matrix4x4 viewProjection)
        {
            for (int i = -20; i <= 20; i++)
            {
                var a = new Vector3(i, 0, -20);
                var b = new Vector3(i, 0, 20);
                DrawWorldLine(context, rect, viewProjection, a, b, i == 0 ? Color.Parse("#B84D7CFF") : (Math.Abs(i) % 5 == 0 ? Color.Parse("#505A7DB4") : Color.Parse("#2A45628C")), i == 0 ? 1.6 : 1.0);
            }

            for (int i = -20; i <= 20; i++)
            {
                var a = new Vector3(-20, 0, i);
                var b = new Vector3(20, 0, i);
                DrawWorldLine(context, rect, viewProjection, a, b, i == 0 ? Color.Parse("#B853D18A") : (Math.Abs(i) % 5 == 0 ? Color.Parse("#505A7DB4") : Color.Parse("#2A45628C")), i == 0 ? 1.6 : 1.0);
            }
        }

        private void DrawOriginAxes(DrawingContext context, Rect rect, Matrix4x4 viewProjection)
        {
            var origin = new Vector3(0, 0, 0);
            DrawWorldLine(context, rect, viewProjection, origin, new Vector3(3.0f, 0, 0), Color.Parse("#E95858"), 2.0);
            DrawWorldLine(context, rect, viewProjection, origin, new Vector3(0, 3.0f, 0), Color.Parse("#6DD47D"), 2.0);
            DrawWorldLine(context, rect, viewProjection, origin, new Vector3(0, 0, 3.0f), Color.Parse("#619FFF"), 2.0);
        }

        private void DrawSelectionCube(DrawingContext context, Rect rect, Matrix4x4 viewProjection)
        {
            var local = new[]
            {
                new Vector3(-0.5f, -0.5f, -0.5f),
                new Vector3( 0.5f, -0.5f, -0.5f),
                new Vector3( 0.5f,  0.5f, -0.5f),
                new Vector3(-0.5f,  0.5f, -0.5f),
                new Vector3(-0.5f, -0.5f,  0.5f),
                new Vector3( 0.5f, -0.5f,  0.5f),
                new Vector3( 0.5f,  0.5f,  0.5f),
                new Vector3(-0.5f,  0.5f,  0.5f),
            };

            var world = new Point?[8];
            var rotation = Quaternion.CreateFromYawPitchRoll(
                DegreesToRadians(_selectionRotationDeg.Y),
                DegreesToRadians(_selectionRotationDeg.X),
                DegreesToRadians(_selectionRotationDeg.Z));

            for (int i = 0; i < local.Length; i++)
            {
                var transformed = Vector3.Transform(local[i] * _selectionScale, rotation) + _selectionPosition;
                if (TryProject(transformed, rect, viewProjection, out var point))
                {
                    world[i] = point;
                }
            }

            FillFace(context, world, new[] { 0, 1, 2, 3 }, Color.Parse("#7F5E8EC6"));
            FillFace(context, world, new[] { 1, 5, 6, 2 }, Color.Parse("#6D4D78AD"));
            FillFace(context, world, new[] { 3, 2, 6, 7 }, Color.Parse("#8E78BDE0"));

            var pen = new Pen(new SolidColorBrush(Color.Parse("#D7E9FF")), 1.5);
            DrawEdge(context, pen, world, 0, 1);
            DrawEdge(context, pen, world, 1, 2);
            DrawEdge(context, pen, world, 2, 3);
            DrawEdge(context, pen, world, 3, 0);
            DrawEdge(context, pen, world, 4, 5);
            DrawEdge(context, pen, world, 5, 6);
            DrawEdge(context, pen, world, 6, 7);
            DrawEdge(context, pen, world, 7, 4);
            DrawEdge(context, pen, world, 0, 4);
            DrawEdge(context, pen, world, 1, 5);
            DrawEdge(context, pen, world, 2, 6);
            DrawEdge(context, pen, world, 3, 7);
        }

        private void DrawGizmo(DrawingContext context, Rect rect, Matrix4x4 viewProjection)
        {
            var origin = _selectionPosition;
            var basis = CreateGizmoBasis();
            var axisLength = ComputeGizmoLength();

            foreach (var axis in new[] { GizmoAxis.X, GizmoAxis.Y, GizmoAxis.Z })
            {
                var dir = axis == GizmoAxis.X ? basis.X : axis == GizmoAxis.Y ? basis.Y : basis.Z;
                var color = axis == GizmoAxis.X ? Color.Parse("#E95858") : axis == GizmoAxis.Y ? Color.Parse("#6DD47D") : Color.Parse("#619FFF");
                var end = origin + dir * axisLength;
                DrawWorldLine(context, rect, viewProjection, origin, end, color, _activeAxis == axis ? 3.4 : 2.6);

                if (!TryProject(end, rect, viewProjection, out var handlePoint))
                {
                    continue;
                }

                if (Operation == ViewportOp.Scale)
                {
                    context.FillRectangle(new SolidColorBrush(color), new Rect(handlePoint.X - 5, handlePoint.Y - 5, 10, 10));
                }
                else if (Operation == ViewportOp.Rotate)
                {
                    context.DrawEllipse(null, new Pen(new SolidColorBrush(color), _activeAxis == axis ? 3 : 2), handlePoint, 6, 6);
                }
                else
                {
                    DrawArrowHead(context, handlePoint, color);
                }
            }

            if (TryProject(origin, rect, viewProjection, out var originPoint))
            {
                context.DrawEllipse(new SolidColorBrush(Color.Parse("#F5F7FA")), null, originPoint, 3.5, 3.5);
            }
        }

        private void DrawViewCube(DrawingContext context, Rect rect)
        {
            var cubeRect = new Rect(rect.Right - 86, rect.Top + 54, 64, 64);
            context.FillRectangle(new SolidColorBrush(Color.FromArgb(64, 12, 18, 28)), cubeRect);
            context.DrawRectangle(new Pen(new SolidColorBrush(Color.FromArgb(90, 255, 255, 255)), 1), cubeRect);
            DrawLabel(context, "Y", new Point(cubeRect.Center.X - 4, cubeRect.Top + 4), 11, new SolidColorBrush(Color.Parse("#6DD47D")));
            DrawLabel(context, "X", new Point(cubeRect.Right - 14, cubeRect.Center.Y - 6), 11, new SolidColorBrush(Color.Parse("#E95858")));
            DrawLabel(context, "Z", new Point(cubeRect.Left + 6, cubeRect.Bottom - 18), 11, new SolidColorBrush(Color.Parse("#619FFF")));
        }

        private Matrix4x4 CreateViewMatrix()
        {
            var yaw = DegreesToRadians((float)_yawDeg);
            var pitch = DegreesToRadians((float)_pitchDeg);
            var forward = new Vector3(
                MathF.Cos(pitch) * MathF.Cos(yaw),
                MathF.Sin(pitch),
                MathF.Cos(pitch) * MathF.Sin(yaw));

            var right = Vector3.Normalize(Vector3.Cross(forward, Vector3.UnitY));
            var up = Vector3.Normalize(Vector3.Cross(right, forward));
            var target = _focus + right * _panOffset.X + up * _panOffset.Y;
            var position = target - forward * (float)_distance;
            return Matrix4x4.CreateLookAt(position, target, Vector3.UnitY);
        }

        private Matrix4x4 CreateProjectionMatrix()
        {
            float aspect = Bounds.Height <= 1 ? 1.0f : (float)(Bounds.Width / Bounds.Height);
            if (_orthographic)
            {
                const float orthoSpan = 12.0f;
                return Matrix4x4.CreateOrthographic(orthoSpan * aspect, orthoSpan, 0.1f, 1000.0f);
            }

            return Matrix4x4.CreatePerspectiveFieldOfView(DegreesToRadians(55.0f), aspect, 0.1f, 1000.0f);
        }

        private (Vector3 X, Vector3 Y, Vector3 Z) CreateGizmoBasis()
        {
            if (Space == ViewportSpace.World)
            {
                return (Vector3.UnitX, Vector3.UnitY, Vector3.UnitZ);
            }

            var rotation = Quaternion.CreateFromYawPitchRoll(
                DegreesToRadians(_selectionRotationDeg.Y),
                DegreesToRadians(_selectionRotationDeg.X),
                DegreesToRadians(_selectionRotationDeg.Z));
            return (
                Vector3.Normalize(Vector3.Transform(Vector3.UnitX, rotation)),
                Vector3.Normalize(Vector3.Transform(Vector3.UnitY, rotation)),
                Vector3.Normalize(Vector3.Transform(Vector3.UnitZ, rotation)));
        }

        private float ComputeGizmoLength() => Math.Max(1.5f, (float)_distance * 0.14f);

        private bool TryProject(Vector3 world, Rect rect, Matrix4x4 viewProjection, out Point point)
        {
            var clip = Vector4.Transform(new Vector4(world, 1.0f), viewProjection);
            if (Math.Abs(clip.W) < 0.0001f)
            {
                point = default;
                return false;
            }

            var ndc = clip / clip.W;
            if (ndc.Z < -1.1f || ndc.Z > 1.1f)
            {
                point = default;
                return false;
            }

            point = new Point(
                rect.X + (ndc.X * 0.5 + 0.5) * rect.Width,
                rect.Y + (-ndc.Y * 0.5 + 0.5) * rect.Height);
            return true;
        }

        private void DrawWorldLine(DrawingContext context, Rect rect, Matrix4x4 viewProjection, Vector3 a, Vector3 b, Color color, double thickness)
        {
            if (!TryProject(a, rect, viewProjection, out var pa) || !TryProject(b, rect, viewProjection, out var pb))
            {
                return;
            }

            context.DrawLine(new Pen(new SolidColorBrush(color), thickness), pa, pb);
        }

        private static void FillFace(DrawingContext context, Point?[] points, int[] indices, Color color)
        {
            if (Array.Exists(indices, i => !points[i].HasValue))
            {
                return;
            }

            var geometry = new StreamGeometry();
            using (var g = geometry.Open())
            {
                g.BeginFigure(points[indices[0]]!.Value, true);
                for (int i = 1; i < indices.Length; i++)
                {
                    g.LineTo(points[indices[i]]!.Value);
                }
                g.EndFigure(true);
            }

            context.DrawGeometry(new SolidColorBrush(color), null, geometry);
        }

        private static void DrawEdge(DrawingContext context, Pen pen, Point?[] points, int a, int b)
        {
            if (!points[a].HasValue || !points[b].HasValue)
            {
                return;
            }
            context.DrawLine(pen, points[a]!.Value, points[b]!.Value);
        }

        private static void DrawArrowHead(DrawingContext context, Point point, Color color)
        {
            context.DrawEllipse(new SolidColorBrush(color), null, point, 4.5, 4.5);
        }

        private static void DrawLabel(DrawingContext context, string text, Point pt, double size, IBrush brush)
        {
            context.DrawText(
                new FormattedText(
                    text,
                    System.Globalization.CultureInfo.InvariantCulture,
                    FlowDirection.LeftToRight,
                    new Typeface("Segoe UI"),
                    size,
                    brush),
                pt);
        }

        private void OnPressed(object? sender, PointerPressedEventArgs e)
        {
            Focus();
            var point = e.GetPosition(this);
            _lastPointer = point;

            var props = e.GetCurrentPoint(this).Properties;
            if (props.IsRightButtonPressed)
            {
                _orbit = true;
            }
            else if (props.IsMiddleButtonPressed)
            {
                _pan = true;
            }
            else if (props.IsLeftButtonPressed)
            {
                _activeAxis = HitTestGizmoAxis(point);
                if (_activeAxis != GizmoAxis.None)
                {
                    _draggingGizmo = true;
                    _dragStartScreen = point;
                    _dragStartPosition = _selectionPosition;
                    _dragStartScale = _selectionScale;
                    _dragStartRotationDeg = _selectionRotationDeg;
                }
            }

            e.Pointer.Capture(this);
        }

        private void OnMoved(object? sender, PointerEventArgs e)
        {
            var point = e.GetPosition(this);
            if (_lastPointer is null)
            {
                _lastPointer = point;
                return;
            }

            var delta = point - _lastPointer.Value;
            _lastPointer = point;

            if (_draggingGizmo && _activeAxis != GizmoAxis.None)
            {
                ApplyGizmoDrag(point);
                ChromeChanged?.Invoke(this, EventArgs.Empty);
                InvalidateVisual();
                return;
            }

            if (_orbit)
            {
                _yawDeg += delta.X * 0.35;
                _pitchDeg = Math.Clamp(_pitchDeg - delta.Y * 0.25, -89.0, 89.0);
                ChromeChanged?.Invoke(this, EventArgs.Empty);
                InvalidateVisual();
            }
            else if (_pan)
            {
                _panOffset.X += (float)(delta.X / Math.Max(1.0, Bounds.Width) * 8.0);
                _panOffset.Y -= (float)(delta.Y / Math.Max(1.0, Bounds.Height) * 8.0);
                ChromeChanged?.Invoke(this, EventArgs.Empty);
                InvalidateVisual();
            }
        }

        private void OnReleased(object? sender, PointerReleasedEventArgs e)
        {
            _orbit = false;
            _pan = false;
            _draggingGizmo = false;
            _activeAxis = GizmoAxis.None;
            _lastPointer = null;
            e.Pointer.Capture(null);
            InvalidateVisual();
        }

        private void OnWheel(object? sender, PointerWheelEventArgs e)
        {
            _distance = Math.Clamp(_distance - e.Delta.Y * 0.6, 2.5, 60.0);
            ChromeChanged?.Invoke(this, EventArgs.Empty);
            InvalidateVisual();
        }

        private void OnKeyDown(object? sender, KeyEventArgs e)
        {
            switch (e.Key)
            {
            case Key.T:
                Operation = ViewportOp.Translate;
                break;
            case Key.E:
                Operation = ViewportOp.Rotate;
                break;
            case Key.R:
                Operation = ViewportOp.Scale;
                break;
            case Key.D1:
                SetPresetView(ViewPreset.Perspective);
                break;
            case Key.D3:
                SetPresetView(ViewPreset.Front);
                break;
            case Key.D7:
                SetPresetView(ViewPreset.Top);
                break;
            case Key.D9:
                SetPresetView(ViewPreset.Isometric);
                break;
            }

            ChromeChanged?.Invoke(this, EventArgs.Empty);
            InvalidateVisual();
        }

        private GizmoAxis HitTestGizmoAxis(Point point)
        {
            var rect = new Rect(Bounds.Size);
            var viewProjection = CreateViewMatrix() * CreateProjectionMatrix();
            var basis = CreateGizmoBasis();
            var origin = _selectionPosition;
            var length = ComputeGizmoLength();

            double best = 12.0;
            GizmoAxis hit = GizmoAxis.None;

            foreach (var axis in new[] { GizmoAxis.X, GizmoAxis.Y, GizmoAxis.Z })
            {
                var dir = axis == GizmoAxis.X ? basis.X : axis == GizmoAxis.Y ? basis.Y : basis.Z;
                if (!TryProject(origin, rect, viewProjection, out var a) || !TryProject(origin + dir * length, rect, viewProjection, out var b))
                {
                    continue;
                }

                var dist = DistanceToSegment(point, a, b);
                if (dist < best)
                {
                    best = dist;
                    hit = axis;
                }
            }

            return hit;
        }

        private void ApplyGizmoDrag(Point currentPoint)
        {
            var rect = new Rect(Bounds.Size);
            var viewProjection = CreateViewMatrix() * CreateProjectionMatrix();
            var basis = CreateGizmoBasis();
            var axisDir = _activeAxis == GizmoAxis.X ? basis.X : _activeAxis == GizmoAxis.Y ? basis.Y : basis.Z;
            var length = ComputeGizmoLength();

            if (!TryProject(_dragStartPosition, rect, viewProjection, out var originScreen) ||
                !TryProject(_dragStartPosition + axisDir * length, rect, viewProjection, out var axisScreen))
            {
                return;
            }

            var axis2d = new AvaloniaVector((axisScreen.X - originScreen.X), (axisScreen.Y - originScreen.Y));
            if (axis2d.Length < 0.001)
            {
                return;
            }

            axis2d = axis2d / axis2d.Length;
            var pointerDelta = currentPoint - _dragStartScreen;
            var alongAxisPixels = pointerDelta.X * axis2d.X + pointerDelta.Y * axis2d.Y;
            var worldDelta = (float)(alongAxisPixels / Math.Max(24.0, axis2d.Length * 48.0)) * length;

            if (Operation == ViewportOp.Translate)
            {
                _selectionPosition = _dragStartPosition + axisDir * worldDelta;
            }
            else if (Operation == ViewportOp.Scale)
            {
                var scaleDelta = Vector3.Zero;
                if (_activeAxis == GizmoAxis.X) scaleDelta.X = worldDelta;
                if (_activeAxis == GizmoAxis.Y) scaleDelta.Y = worldDelta;
                if (_activeAxis == GizmoAxis.Z) scaleDelta.Z = worldDelta;
                _selectionScale = Vector3.Max(new Vector3(0.2f), _dragStartScale + scaleDelta);
            }
            else
            {
                const float rotationFactor = 0.65f;
                if (_activeAxis == GizmoAxis.X) _selectionRotationDeg.X = _dragStartRotationDeg.X + worldDelta * rotationFactor * 35.0f;
                if (_activeAxis == GizmoAxis.Y) _selectionRotationDeg.Y = _dragStartRotationDeg.Y + worldDelta * rotationFactor * 35.0f;
                if (_activeAxis == GizmoAxis.Z) _selectionRotationDeg.Z = _dragStartRotationDeg.Z + worldDelta * rotationFactor * 35.0f;
            }
        }

        private void Poll()
        {
            if (EngineHandle == 0)
            {
                _textureId = 0;
                _stats = default;
                _chunks = 0;
                _seed = 0;
                return;
            }

            try
            {
                _textureId = Native.bm_engine_scene_texture(EngineHandle);
                _chunks = Native.bm_world_chunk_count(EngineHandle);
                _seed = Native.bm_world_get_seed(EngineHandle);
                Native.bm_stats_get(EngineHandle, out var ch, out var dc, out var tr, out var ms);
                _stats = new RenderStats { ChunksDrawn = ch, DrawCalls = dc, Triangles = tr, FrameMs = ms };
            }
            catch
            {
                _textureId = 0;
            }
        }

        private void ResizeNative(int w, int h)
        {
            if (EngineHandle == 0 || w <= 0 || h <= 0)
            {
                return;
            }

            try
            {
                Native.bm_engine_resize_viewport(EngineHandle, w, h);
            }
            catch
            {
            }
        }

        private static double DistanceToSegment(Point p, Point a, Point b)
        {
            var ab = new AvaloniaVector(b.X - a.X, b.Y - a.Y);
            var ap = new AvaloniaVector(p.X - a.X, p.Y - a.Y);
            var lengthSq = ab.X * ab.X + ab.Y * ab.Y;
            if (lengthSq <= 0.0001)
            {
                return Math.Sqrt(ap.X * ap.X + ap.Y * ap.Y);
            }

            var t = Math.Clamp((ap.X * ab.X + ap.Y * ab.Y) / lengthSq, 0.0, 1.0);
            var closest = new Point(a.X + ab.X * t, a.Y + ab.Y * t);
            var dx = p.X - closest.X;
            var dy = p.Y - closest.Y;
            return Math.Sqrt(dx * dx + dy * dy);
        }

        private static float DegreesToRadians(float degrees) => degrees * (MathF.PI / 180.0f);

        private static LinearGradientBrush CreateBackgroundBrush() => new()
        {
            StartPoint = new RelativePoint(0, 0, RelativeUnit.Relative),
            EndPoint = new RelativePoint(0, 1, RelativeUnit.Relative),
            GradientStops = new GradientStops
            {
                new GradientStop(Color.FromRgb(18, 26, 40), 0),
                new GradientStop(Color.FromRgb(11, 14, 20), 1),
            }
        };
    }
}
