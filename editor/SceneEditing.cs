using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Runtime.CompilerServices;
using System.Text.Json;

namespace Balmung.Editor
{
    public sealed class SceneEditorState : INotifyPropertyChanged
    {
        private const string BalmungSceneExtension = ".bmsc";
        private const string LegacyBalmungSceneExtension = ".balmung";
        private const string LegacyJsonSceneExtension = ".json";
        private readonly Dictionary<(int X, int Y, int Z), SceneBlockData> _blocks = new();
        private readonly SceneCellModel[,] _cells;
        private string _sceneName = "scene1";
        private bool _eraseMode;
        private int _layerY = 20;
        private int _brushSize = 1;
        private string _selectedBlockName = "stone";
        private byte _selectedR = 180;
        private byte _selectedG = 180;
        private byte _selectedB = 180;

        public event PropertyChangedEventHandler? PropertyChanged;
        public event Action? SceneChanged;
        public event Action<ScenePaintOp>? CellPainted;

        public int Columns { get; }
        public int Rows { get; }

        public string SceneName
        {
            get => _sceneName;
            set => Set(ref _sceneName, string.IsNullOrWhiteSpace(value) ? "scene1" : value.Trim());
        }

        public bool EraseMode
        {
            get => _eraseMode;
            set
            {
                if (Set(ref _eraseMode, value))
                    OnPropertyChanged(nameof(ActiveToolLabel));
            }
        }

        public int LayerY
        {
            get => _layerY;
            set
            {
                if (Set(ref _layerY, Math.Clamp(value, 0, 255)))
                {
                    OnPropertyChanged(nameof(LayerYText));
                    _rebuildLayerSliceFromBlocks();
                    SceneChanged?.Invoke();
                }
            }
        }

        public string LayerYText
        {
            get => _layerY.ToString();
            set
            {
                if (!int.TryParse(value, out int parsed)) return;
                LayerY = parsed;
            }
        }

        public string SelectedBlockName => _selectedBlockName;
        public byte SelectedR => _selectedR;
        public byte SelectedG => _selectedG;
        public byte SelectedB => _selectedB;
        public int BrushSize
        {
            get => _brushSize;
            set => Set(ref _brushSize, Math.Clamp(value, 1, 8));
        }

        public string ActiveToolLabel => EraseMode ? "Erase" : $"Paint: {_selectedBlockName}";

        public SceneEditorState(int columns = 24, int rows = 24)
        {
            Columns = Math.Max(1, columns);
            Rows = Math.Max(1, rows);
            _cells = new SceneCellModel[Rows, Columns];
            for (int z = 0; z < Rows; z++)
            for (int x = 0; x < Columns; x++)
                _cells[z, x] = new SceneCellModel(x, z);
        }

        public SceneCellModel GetCell(int x, int z) => _cells[z, x];

        public void SetSelectedBlock(string? blockName, byte r, byte g, byte b)
        {
            _selectedBlockName = string.IsNullOrWhiteSpace(blockName) ? "stone" : blockName.Trim();
            _selectedR = r;
            _selectedG = g;
            _selectedB = b;
            OnPropertyChanged(nameof(SelectedBlockName));
            OnPropertyChanged(nameof(SelectedR));
            OnPropertyChanged(nameof(SelectedG));
            OnPropertyChanged(nameof(SelectedB));
            OnPropertyChanged(nameof(ActiveToolLabel));
        }

        public void ToggleEraseMode() => EraseMode = !EraseMode;

        public bool PaintAt(int x, int z)
        {
            if (x < 0 || z < 0 || x >= Columns || z >= Rows) return false;
            var cell = _cells[z, x];
            var key = (x, LayerY, z);

            if (EraseMode)
            {
                if (cell.IsEmpty) return false;
                cell.SetEmpty();
                _blocks.Remove(key);
                SceneChanged?.Invoke();
                CellPainted?.Invoke(new ScenePaintOp(x, z, null));
                return true;
            }

            if (cell.BlockName == _selectedBlockName &&
                cell.R == _selectedR && cell.G == _selectedG && cell.B == _selectedB)
                return false;

            cell.SetBlock(_selectedBlockName, _selectedR, _selectedG, _selectedB);
            _blocks[key] = new SceneBlockData(_selectedBlockName, _selectedR, _selectedG, _selectedB);
            SceneChanged?.Invoke();
            CellPainted?.Invoke(new ScenePaintOp(x, z, _selectedBlockName));
            return true;
        }

        public int PaintBrushAt(int centerX, int centerZ)
        {
            int size = Math.Max(1, BrushSize);
            int radius = size - 1;
            int changed = 0;
            for (int z = centerZ - radius; z <= centerZ + radius; z++)
            for (int x = centerX - radius; x <= centerX + radius; x++)
            {
                if (PaintAt(x, z))
                    changed++;
            }
            return changed;
        }

        public int FillCurrentLayer()
        {
            int changed = 0;
            for (int z = 0; z < Rows; z++)
            for (int x = 0; x < Columns; x++)
            {
                if (PaintAt(x, z))
                    changed++;
            }
            return changed;
        }

        public int FloodFillAt(int startX, int startZ)
        {
            if (startX < 0 || startZ < 0 || startX >= Columns || startZ >= Rows) return 0;
            var start = _cells[startZ, startX];
            bool targetEmpty = start.IsEmpty;
            string targetName = start.BlockName;
            byte tr = start.R, tg = start.G, tb = start.B;

            if (!EraseMode &&
                !targetEmpty &&
                targetName == _selectedBlockName && tr == _selectedR && tg == _selectedG && tb == _selectedB)
                return 0;
            if (EraseMode && targetEmpty)
                return 0;

            var q = new Queue<(int X, int Z)>();
            var seen = new HashSet<(int X, int Z)>();
            q.Enqueue((startX, startZ));
            int changed = 0;

            while (q.Count > 0)
            {
                var p = q.Dequeue();
                if (!seen.Add(p)) continue;
                if (!MatchesTarget(p.X, p.Z, targetEmpty, targetName, tr, tg, tb)) continue;
                if (PaintAt(p.X, p.Z))
                    changed++;

                TryEnqueue(p.X + 1, p.Z, q);
                TryEnqueue(p.X - 1, p.Z, q);
                TryEnqueue(p.X, p.Z + 1, q);
                TryEnqueue(p.X, p.Z - 1, q);
            }

            return changed;
        }

        public int DrawLine(int x0, int z0, int x1, int z1)
        {
            int changed = 0;
            int dx = Math.Abs(x1 - x0);
            int sx = x0 < x1 ? 1 : -1;
            int dz = -Math.Abs(z1 - z0);
            int sz = z0 < z1 ? 1 : -1;
            int err = dx + dz;

            while (true)
            {
                changed += PaintBrushAt(x0, z0) > 0 ? 1 : 0;
                if (x0 == x1 && z0 == z1) break;
                int e2 = 2 * err;
                if (e2 >= dz)
                {
                    err += dz;
                    x0 += sx;
                }
                if (e2 <= dx)
                {
                    err += dx;
                    z0 += sz;
                }
            }
            return changed;
        }

        public void Clear()
        {
            bool changed = false;
            for (int z = 0; z < Rows; z++)
            for (int x = 0; x < Columns; x++)
            {
                if (_cells[z, x].IsEmpty) continue;
                _cells[z, x].SetEmpty();
                _blocks.Remove((x, LayerY, z));
                changed = true;
            }

            if (changed)
                SceneChanged?.Invoke();
        }

        public SceneDocument ToDocument()
        {
            var doc = new SceneDocument
            {
                Name = SceneName,
                LayerY = LayerY,
                Columns = Columns,
                Rows = Rows,
            };

            for (int z = 0; z < Rows; z++)
            for (int x = 0; x < Columns; x++)
            {
                var c = _cells[z, x];
                if (c.IsEmpty) continue;
                doc.Blocks.Add(new SceneBlockPlacement
                {
                    X = x,
                    Y = LayerY,
                    Z = z,
                    Block = c.BlockName,
                    R = c.R,
                    G = c.G,
                    B = c.B,
                });
            }

            foreach (var kvp in _blocks)
            {
                if (kvp.Key.Y == LayerY) continue;
                doc.Blocks.Add(new SceneBlockPlacement
                {
                    X = kvp.Key.X,
                    Y = kvp.Key.Y,
                    Z = kvp.Key.Z,
                    Block = kvp.Value.Name,
                    R = kvp.Value.R,
                    G = kvp.Value.G,
                    B = kvp.Value.B,
                });
            }

            return doc;
        }

        public void LoadDocument(SceneDocument doc)
        {
            _blocks.Clear();
            for (int z = 0; z < Rows; z++)
            for (int x = 0; x < Columns; x++)
                _cells[z, x].SetEmpty();

            SceneName = string.IsNullOrWhiteSpace(doc.Name) ? SceneName : doc.Name;
            LayerY = doc.LayerY;
            foreach (var b in doc.Blocks)
            {
                if (b.X < 0 || b.Z < 0 || b.X >= Columns || b.Z >= Rows) continue;
                int by = Math.Clamp(b.Y, 0, 255);
                _blocks[(b.X, by, b.Z)] = new SceneBlockData(b.Block, b.R, b.G, b.B);
            }

            _rebuildLayerSliceFromBlocks();
            SceneChanged?.Invoke();
        }

        public string GetScenePath(string projectRoot)
        {
            var safe = string.Concat(SceneName.Split(Path.GetInvalidFileNameChars(), StringSplitOptions.RemoveEmptyEntries));
            if (string.IsNullOrWhiteSpace(safe)) safe = "scene1";
            return Path.Combine(projectRoot, "Scenes", safe + BalmungSceneExtension);
        }

        public string GetLegacyScenePath(string projectRoot)
        {
            var safe = string.Concat(SceneName.Split(Path.GetInvalidFileNameChars(), StringSplitOptions.RemoveEmptyEntries));
            if (string.IsNullOrWhiteSpace(safe)) safe = "scene1";
            return Path.Combine(projectRoot, "Scenes", safe + LegacyJsonSceneExtension);
        }

        public string GetLegacyBalmungScenePath(string projectRoot)
        {
            var safe = string.Concat(SceneName.Split(Path.GetInvalidFileNameChars(), StringSplitOptions.RemoveEmptyEntries));
            if (string.IsNullOrWhiteSpace(safe)) safe = "scene1";
            return Path.Combine(projectRoot, "Scenes", safe + LegacyBalmungSceneExtension);
        }

        public (bool ok, string message) SaveToProject(string projectRoot)
        {
            try
            {
                Directory.CreateDirectory(Path.Combine(projectRoot, "Scenes"));
                var path = GetScenePath(projectRoot);
                var payload = new BmscSceneFile
                {
                    SchemaVersion = 1,
                    Scene = ToDocument(),
                };
                var text = JsonSerializer.Serialize(payload, SceneJson.Options);
                File.WriteAllText(path, text);
                return (true, path);
            }
            catch (Exception ex)
            {
                return (false, ex.Message);
            }
        }

        public (bool ok, string message) LoadFromProject(string projectRoot)
        {
            try
            {
                var path = GetScenePath(projectRoot);
                if (!File.Exists(path))
                {
                    string legacyBalmungPath = GetLegacyBalmungScenePath(projectRoot);
                    if (File.Exists(legacyBalmungPath))
                    {
                        path = legacyBalmungPath;
                    }
                    else
                    {
                        string legacyJsonPath = GetLegacyScenePath(projectRoot);
                        if (!File.Exists(legacyJsonPath)) return (false, $"Scene file not found: {path}");
                        path = legacyJsonPath;
                    }
                }

                var json = File.ReadAllText(path);
                SceneDocument? doc = null;
                using (var parsed = JsonDocument.Parse(json))
                {
                    if (parsed.RootElement.TryGetProperty("scene", out var sceneElement))
                    {
                        doc = sceneElement.Deserialize<SceneDocument>(SceneJson.Options);
                    }
                    else
                    {
                        doc = JsonSerializer.Deserialize<SceneDocument>(json, SceneJson.Options);
                    }
                }

                if (doc is null) return (false, "Scene file is empty or invalid.");
                if (doc.Columns != Columns || doc.Rows != Rows)
                {
                    // Keep editor grid size stable; ignore mismatch and clamp placements.
                }
                LoadDocument(doc);
                return (true, path);
            }
            catch (Exception ex)
            {
                return (false, ex.Message);
            }
        }

        private bool Set<T>(ref T field, T value, [CallerMemberName] string prop = "")
        {
            if (EqualityComparer<T>.Default.Equals(field, value)) return false;
            field = value;
            OnPropertyChanged(prop);
            return true;
        }

        private bool MatchesTarget(int x, int z, bool targetEmpty, string targetName, byte tr, byte tg, byte tb)
        {
            var c = _cells[z, x];
            if (targetEmpty) return c.IsEmpty;
            return !c.IsEmpty &&
                   c.BlockName == targetName &&
                   c.R == tr && c.G == tg && c.B == tb;
        }

        private void TryEnqueue(int x, int z, Queue<(int X, int Z)> q)
        {
            if (x < 0 || z < 0 || x >= Columns || z >= Rows) return;
            q.Enqueue((x, z));
        }

        private void _rebuildLayerSliceFromBlocks()
        {
            for (int z = 0; z < Rows; z++)
            for (int x = 0; x < Columns; x++)
                _cells[z, x].SetEmpty();

            foreach (var kvp in _blocks)
            {
                if (kvp.Key.Y != LayerY) continue;
                _cells[kvp.Key.Z, kvp.Key.X].SetBlock(kvp.Value.Name, kvp.Value.R, kvp.Value.G, kvp.Value.B);
            }
        }

        private void OnPropertyChanged([CallerMemberName] string prop = "")
            => PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(prop));
    }

    public sealed class SceneCellModel
    {
        public int X { get; }
        public int Z { get; }
        public string BlockName { get; private set; } = "";
        public byte R { get; private set; }
        public byte G { get; private set; }
        public byte B { get; private set; }
        public bool IsEmpty => string.IsNullOrEmpty(BlockName);

        public SceneCellModel(int x, int z)
        {
            X = x;
            Z = z;
        }

        public void SetBlock(string name, byte r, byte g, byte b)
        {
            BlockName = name ?? "";
            R = r; G = g; B = b;
        }

        public void SetEmpty()
        {
            BlockName = "";
            R = G = B = 0;
        }
    }

    public readonly record struct ScenePaintOp(int X, int Z, string? BlockName);

    public sealed class SceneDocument
    {
        public string Name { get; set; } = "scene1";
        public int LayerY { get; set; } = 20;
        public int Columns { get; set; } = 24;
        public int Rows { get; set; } = 24;
        public List<SceneBlockPlacement> Blocks { get; set; } = new();
    }

    public sealed class BmscSceneFile
    {
        public string Format { get; set; } = "BalmungScene";
        public string Kind { get; set; } = "scene";
        public string Engine { get; set; } = "Balmung Engine";
        public int SchemaVersion { get; set; } = 1;
        public SceneDocument Scene { get; set; } = new();
    }

    public sealed class SceneBlockPlacement
    {
        public int X { get; set; }
        public int Y { get; set; } = 20;
        public int Z { get; set; }
        public string Block { get; set; } = "";
        public byte R { get; set; }
        public byte G { get; set; }
        public byte B { get; set; }
    }

    internal readonly record struct SceneBlockData(string Name, byte R, byte G, byte B);

    internal static class SceneJson
    {
        public static readonly JsonSerializerOptions Options = new()
        {
            WriteIndented = true,
            PropertyNamingPolicy = JsonNamingPolicy.CamelCase,
        };
    }
}


