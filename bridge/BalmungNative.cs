// Balmung.Bridge — BalmungNative.cs
// P/Invoke declarations that mirror balmung_capi.h exactly.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Runtime.Loader;
using System.Text;

namespace Balmung.Bridge
{
    public static class Native
    {
        private const string DLL = "balmung";

        static Native()
        {
            NativeLibrary.SetDllImportResolver(typeof(Native).Assembly, Resolve);
        }

        private static IntPtr Resolve(string libraryName, Assembly assembly, DllImportSearchPath? searchPath)
        {
            if (!string.Equals(libraryName, DLL, StringComparison.OrdinalIgnoreCase))
            {
                return IntPtr.Zero;
            }

            string baseDir = AppContext.BaseDirectory;
            string[] dllNames =
            {
                "balmung.dll",
                "libbalmung.dll",
            };

            string[] relativeRoots =
            {
                ".",
                Path.Combine("runtimes", "win-x64", "native"),
                "release",
                Path.Combine("release", "balmung"),
                Path.Combine("release", "bal-editor"),
            };

            string[] buildDirs =
            {
                "build-2.4.0",
                "build-balmung",
                "build-balmung-gpu",
                "build-balmung-gpu2",
                "build-native",
                "build-export",
                "build-voxel",
            };

            var candidates = new List<string>();
            for (int depth = 0; depth <= 6; depth++)
            {
                string up = baseDir;
                for (int i = 0; i < depth; i++)
                {
                    up = Path.GetFullPath(Path.Combine(up, ".."));
                }

                foreach (string root in relativeRoots)
                {
                    foreach (string dll in dllNames)
                    {
                        candidates.Add(Path.GetFullPath(Path.Combine(up, root, dll)));
                    }
                }

                foreach (string buildDir in buildDirs)
                {
                    foreach (string dll in dllNames)
                    {
                        candidates.Add(Path.GetFullPath(Path.Combine(up, buildDir, dll)));
                    }
                }
            }

            foreach (string candidate in candidates.Distinct(StringComparer.OrdinalIgnoreCase))
            {
                if (File.Exists(candidate) && NativeLibrary.TryLoad(candidate, out IntPtr handle))
                {
                    return handle;
                }
            }

            return IntPtr.Zero;
        }

        // Lifecycle
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern long bm_engine_create();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void bm_engine_destroy(long handle);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool bm_engine_init_headless(long handle, int vw, int vh);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool bm_engine_init_windowed(long handle, int w, int h,
            [MarshalAs(UnmanagedType.LPStr)] string title);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void bm_engine_tick(long handle, float dt);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool bm_engine_running(long handle);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void bm_engine_quit(long handle);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void bm_engine_set_menu_mode(long handle, [MarshalAs(UnmanagedType.I1)] bool enabled);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void bm_ui_begin(long handle);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void bm_ui_add_rect(long handle, float x, float y, float w, float h, float r, float g, float b, float a, int layer);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void bm_ui_add_text(long handle, float x, float y, float pixelSize, float r, float g, float b, float a, int layer, [MarshalAs(UnmanagedType.LPStr)] string text);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint bm_engine_scene_texture(long handle);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void bm_engine_resize_viewport(long handle, int w, int h);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void bm_renderer_use_visibility_buffer(long handle, [MarshalAs(UnmanagedType.I1)] bool enabled);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void bm_renderer_set_automatic_lod(long handle, [MarshalAs(UnmanagedType.I1)] bool enabled);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int bm_renderer_pipeline_architecture(long handle);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern ulong bm_entity_create(long handle, [MarshalAs(UnmanagedType.LPStr)] string name);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool bm_entity_destroy(long handle, ulong entityId);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int bm_entity_count(long handle);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool bm_entity_set_position(long handle, ulong entityId, float x, float y, float z);

        // World
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint bm_world_get_seed(long handle);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int bm_world_chunk_count(long handle);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool bm_world_place_voxel(long handle, int x, int y, int z,
            [MarshalAs(UnmanagedType.LPStr)] string blockType);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool bm_world_destroy_voxel(long handle, int x, int y, int z);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool bm_world_get_voxel(long handle, int x, int y, int z,
            StringBuilder outBlock, int bufSize);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool bm_world_save(long handle,
            [MarshalAs(UnmanagedType.LPStr)] string name);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool bm_world_load(long handle,
            [MarshalAs(UnmanagedType.LPStr)] string name);

        // Block Registry
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void bm_block_register(long handle,
            [MarshalAs(UnmanagedType.LPStr)] string name,
            byte r, byte g, byte b, float hardness);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int bm_block_count(long handle);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool bm_block_get_name(long handle, int index,
            StringBuilder nameBuf, int bufSize);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool bm_block_get_color(long handle,
            [MarshalAs(UnmanagedType.LPStr)] string name,
            out byte r, out byte g, out byte b);

        // Inventory
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void bm_inv_add_item(long handle,
            [MarshalAs(UnmanagedType.LPStr)] string blockType, int count);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool bm_inv_remove_item(long handle,
            [MarshalAs(UnmanagedType.LPStr)] string blockType, int count);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int bm_inv_item_count(long handle,
            [MarshalAs(UnmanagedType.LPStr)] string blockType);

        // Crafting

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool bm_craft_can_craft(long handle, IntPtr pattern9);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool bm_craft_do_craft(long handle, IntPtr pattern9,
            StringBuilder resultBlock, int bufSize, out int resultCount);

        // Lua Scripting
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int bm_lua_load_mods(long handle,
            [MarshalAs(UnmanagedType.LPStr)] string dir);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool bm_lua_exec(long handle,
            [MarshalAs(UnmanagedType.LPStr)] string code,
            StringBuilder errorBuf, int bufSize);

        // Events
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void EventCallbackDelegate(
            [MarshalAs(UnmanagedType.LPStr)] string eventName,
            [MarshalAs(UnmanagedType.LPStr)] string payloadJson);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void bm_events_subscribe(long handle, EventCallbackDelegate cb);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void bm_events_unsubscribe(long handle, EventCallbackDelegate cb);

        // Stats
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void bm_stats_get(long handle,
            out int chunksDrawn, out int drawCalls,
            out int triangles,   out float frameMs);
    }
}
















