// VoxelBlock.Bridge — VoxelBlockEngine.cs
// High-level C# wrapper.
//
// Fix log:
//   ⑭ CanCraft/Craft: use WithPattern9() helper that manually pins
//      UTF-8 byte arrays and builds a const char*[9] on the native heap,
//      avoiding LPArray marshalling bugs across platforms.

using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace VoxelBlock.Bridge
{
    public sealed class VoxelBlockEngine : IDisposable
    {
        private long   _handle;
        private bool   _disposed;
        private Native.EventCallbackDelegate? _eventDelegate;

        public event Action<string, string>? OnEngineEvent;

        public long Handle  => _handle;
        public bool IsValid => _handle != 0 && !_disposed;

        // ── Lifecycle ────────────────────────────────────────────────────────
        public VoxelBlockEngine()
        {
            _handle = Native.vb_engine_create();
            if (_handle == 0)
                throw new InvalidOperationException("[VBEngine] vb_engine_create() returned 0");
        }

        public bool InitHeadless(int vw = 1280, int vh = 720)
        {
            if (!IsValid) return false;
            bool ok = Native.vb_engine_init_headless(_handle, vw, vh);
            if (ok) _subscribeEvents();
            return ok;
        }

        public bool InitWindowed(int w = 1280, int h = 720, string title = "VoxelBlock")
        {
            if (!IsValid) return false;
            bool ok = Native.vb_engine_init_windowed(_handle, w, h, title);
            if (ok) _subscribeEvents();
            return ok;
        }

        public void Tick(float dt)  { if (IsValid) Native.vb_engine_tick(_handle, dt); }
        public bool IsRunning       => IsValid && Native.vb_engine_running(_handle);
        public void Quit()          { if (IsValid) Native.vb_engine_quit(_handle); }
        public uint SceneTextureId  => IsValid ? Native.vb_engine_scene_texture(_handle) : 0;

        public void ResizeViewport(int w, int h)
        { if (IsValid) Native.vb_engine_resize_viewport(_handle, w, h); }

        // ── World ────────────────────────────────────────────────────────────
        public uint WorldSeed  => IsValid ? Native.vb_world_get_seed(_handle)    : 0;
        public int  ChunkCount => IsValid ? Native.vb_world_chunk_count(_handle) : 0;

        public bool PlaceVoxel(int x, int y, int z, string blockType) =>
            IsValid && Native.vb_world_place_voxel(_handle, x, y, z, blockType);

        public bool DestroyVoxel(int x, int y, int z) =>
            IsValid && Native.vb_world_destroy_voxel(_handle, x, y, z);

        public string? GetVoxel(int x, int y, int z)
        {
            if (!IsValid) return null;
            var sb = new StringBuilder(64);
            return Native.vb_world_get_voxel(_handle, x, y, z, sb, 64) ? sb.ToString() : null;
        }

        public bool SaveWorld(string name) => IsValid && Native.vb_world_save(_handle, name);
        public bool LoadWorld(string name) => IsValid && Native.vb_world_load(_handle, name);

        // ── Block Registry ────────────────────────────────────────────────────
        public void RegisterBlock(string name, byte r, byte g, byte b, float hardness = 1f)
        { if (IsValid) Native.vb_block_register(_handle, name, r, g, b, hardness); }

        public int BlockCount => IsValid ? Native.vb_block_count(_handle) : 0;

        public string? GetBlockName(int idx)
        {
            if (!IsValid) return null;
            var sb = new StringBuilder(64);
            return Native.vb_block_get_name(_handle, idx, sb, 64) ? sb.ToString() : null;
        }

        public (byte r, byte g, byte b)? GetBlockColor(string name)
        {
            if (!IsValid) return null;
            return Native.vb_block_get_color(_handle, name, out byte r, out byte g, out byte b)
                ? (r, g, b) : default((byte, byte, byte)?);
        }

        public List<BlockInfo> GetAllBlocks()
        {
            var list = new List<BlockInfo>();
            int count = BlockCount;
            for (int i = 0; i < count; i++)
            {
                var name = GetBlockName(i);
                if (name is null) continue;
                var col = GetBlockColor(name);
                list.Add(new BlockInfo
                    { Name = name, R = col?.r ?? 128, G = col?.g ?? 128, B = col?.b ?? 128 });
            }
            return list;
        }

        // ── Inventory ─────────────────────────────────────────────────────────
        public void AddItem(string bt, int count = 1)
        { if (IsValid) Native.vb_inv_add_item(_handle, bt, count); }

        public bool RemoveItem(string bt, int count = 1) =>
            IsValid && Native.vb_inv_remove_item(_handle, bt, count);

        public int ItemCount(string bt) =>
            IsValid ? Native.vb_inv_item_count(_handle, bt) : 0;

        // ── Crafting (FIX ⑭) ──────────────────────────────────────────────────
        // Builds const char*[9] manually on the native heap.
        // Each slot is a null-terminated UTF-8 byte[] pinned for the duration of the call.

        public bool CanCraft(string[] pattern9)
        {
            if (!IsValid) return false;
            return _withPattern9(pattern9, ptr => Native.vb_craft_can_craft(_handle, ptr));
        }

        public CraftResult? Craft(string[] pattern9)
        {
            if (!IsValid) return null;
            var sb = new StringBuilder(64);
            CraftResult? result = null;
            _withPattern9(pattern9, ptr =>
            {
                if (!Native.vb_craft_do_craft(_handle, ptr, sb, 64, out int count))
                    return false;
                result = new CraftResult { BlockType = sb.ToString(), Count = count };
                return true;
            });
            return result;
        }

        // Allocates an array of 9 IntPtr (each pointing to a UTF-8 string),
        // calls action(ptrToArray), then frees everything.
        private static T _withPattern9<T>(string[] pattern9, Func<IntPtr, T> action)
        {
            if (pattern9.Length < 9)
                Array.Resize(ref pattern9, 9);

            // Encode each slot as UTF-8 bytes (null-terminated)
            var bufs  = new byte[9][];
            var ptrs  = new IntPtr[9];
            var hPtrs = new IntPtr[9];

            for (int i = 0; i < 9; i++)
            {
                string s = pattern9[i] ?? "";
                bufs[i]  = Encoding.UTF8.GetBytes(s + '\0');
                ptrs[i]  = Marshal.AllocHGlobal(bufs[i].Length);
                Marshal.Copy(bufs[i], 0, ptrs[i], bufs[i].Length);
                hPtrs[i] = ptrs[i];
            }

            // Allocate array of 9 IntPtr on native heap
            IntPtr ptrArray = Marshal.AllocHGlobal(IntPtr.Size * 9);
            try
            {
                for (int i = 0; i < 9; i++)
                    Marshal.WriteIntPtr(ptrArray, i * IntPtr.Size, ptrs[i]);

                return action(ptrArray);
            }
            finally
            {
                Marshal.FreeHGlobal(ptrArray);
                for (int i = 0; i < 9; i++)
                    Marshal.FreeHGlobal(ptrs[i]);
            }
        }

        // ── Lua ───────────────────────────────────────────────────────────────
        public int LoadMods(string dir = "mods") =>
            IsValid ? Native.vb_lua_load_mods(_handle, dir) : 0;

        public (bool ok, string? error) ExecLua(string code)
        {
            if (!IsValid) return (false, "Engine not valid");
            var err = new StringBuilder(1024);
            bool ok = Native.vb_lua_exec(_handle, code, err, 1024);
            return (ok, ok ? null : err.ToString());
        }

        // ── Stats ─────────────────────────────────────────────────────────────
        public RenderStats GetStats()
        {
            if (!IsValid) return default;
            Native.vb_stats_get(_handle,
                out int cd, out int dc, out int tri, out float ms);
            return new RenderStats
                { ChunksDrawn = cd, DrawCalls = dc, Triangles = tri, FrameMs = ms };
        }

        // ── Events ────────────────────────────────────────────────────────────
        private void _subscribeEvents()
        {
            _eventDelegate = (name, json) => OnEngineEvent?.Invoke(name, json);
            Native.vb_events_subscribe(_handle, _eventDelegate);
        }

        // ── IDisposable ───────────────────────────────────────────────────────
        public void Dispose()
        {
            if (_disposed) return;
            _disposed = true;
            if (_handle != 0)
            {
                if (_eventDelegate is not null)
                    Native.vb_events_unsubscribe(_handle, _eventDelegate);
                Native.vb_engine_destroy(_handle);
                _handle = 0;
            }
            GC.SuppressFinalize(this);
        }

        ~VoxelBlockEngine() => Dispose();
    }

    // ── Data types ────────────────────────────────────────────────────────────
    public struct BlockInfo
    {
        public string Name;
        public byte   R, G, B;
    }

    public struct CraftResult
    {
        public string BlockType;
        public int    Count;
    }

    public struct RenderStats
    {
        public int   ChunksDrawn;
        public int   DrawCalls;
        public int   Triangles;
        public float FrameMs;
    }
}
