-- Balmung Engine — Example Mod: mystuff
-- Demonstrates the Lua scripting API

print("[Mod] mystuff loading...")

-- Register a custom block
register_block("obsidian", 30, 20, 50, 5.0)
register_block("mossy_stone", 80, 100, 70, 1.5)

-- Register custom recipe: 4 stone → 1 obsidian
register_recipe(
  {"stone","stone","stone","stone","","","","",""},
  "obsidian", 1
)

-- Listen to events
on("on_place", function()
  print("[Mod] A block was placed!")
end)

on("on_destroy", function()
  print("[Mod] A block was destroyed!")
end)

on("on_craft", function()
  print("[Mod] Something was crafted!")
end)

-- Give player starting items from this mod
give_item("obsidian", 8)
give_item("mossy_stone", 16)

-- Place a structure (a small tower at spawn)
local function place_tower(bx, by, bz, height)
  for y = 0, height do
    place_block(bx,    by + y, bz,    "obsidian")
    place_block(bx+1,  by + y, bz,    "obsidian")
    place_block(bx,    by + y, bz+1,  "obsidian")
    place_block(bx+1,  by + y, bz+1,  "obsidian")
  end
  print("[Mod] Tower placed at " .. bx .. "," .. by .. "," .. bz)
end

-- place_tower(20, 15, 20, 6)   -- uncomment to activate

print("[Mod] mystuff loaded OK")


