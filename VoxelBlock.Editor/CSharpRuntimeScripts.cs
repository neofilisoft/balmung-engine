using VoxelBlock.Bridge.Scripting;

namespace VoxelBlock.Editor
{
    public sealed class EditorHeartbeatScript : VoxelScriptBehaviour
    {
        private float _accum;
        private int _ticks;

        public override void OnStart()
        {
            Log("started");
        }

        public override void OnUpdate(float dt)
        {
            _ticks++;
            _accum += dt;
            if (_accum < 1.0f) return;
            _accum = 0f;
            Log($"tick={_ticks}, engineReady={Context.IsEngineReady}");
        }

        public override void OnDestroy()
        {
            Log("stopped");
        }
    }

    public sealed class LuaBridgeDemoScript : VoxelScriptBehaviour
    {
        private bool _ran;

        public override void OnStart()
        {
            Log("started");
        }

        public override void OnUpdate(float dt)
        {
            if (_ran || !Context.IsEngineReady) return;
            _ran = true;
            var (ok, err) = Context.ExecLua("print('Hello from C# runtime script -> Lua')");
            if (!ok) Log($"Lua bridge call failed: {err}");
            else Log("sent Lua command");
        }
    }
}
