# This is a named root node (technically just a field with a node value):
materials/base/fl_tile1 {
  0 {
    # Incidentally, #s begin comments
    map textures/base/fl_tile1.png # Field with a value
  }
  clamp_u # Value-less fields
  clamp_v
}

# And this is a nameless root node (requires SP_NAMELESS_ROOT_NODES):
{
  reason_for_using none
}

# And you can just pop down stuff anywhere in the root, really
fullscreen
fov 100

# Semicolons can terminate a value and tell the parser to go onto a new field
width 800; height 600