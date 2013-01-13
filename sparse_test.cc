#include "sparse.hh"
#include <iostream>
#include <string>

static void handler(sparse_msg_t msg, const char *start, const char *end, void *context)
{
  std::clog << "MSG " << msg << " [" << std::string(start, (size_t)(end - start)) << ']' << std::endl;
}

int main(int argc, char const *argv[])
{
  std::string test_source =
    "# This is a named root node (technically just a field with a node value):\n"
    "materials/base/fl_tile1 {\n"
    "  0 {\n"
    "    # Incidentally, #s begin comments\n"
    "    map textures/base/fl_tile1.png # Field with a value\n"
    "  }\n"
    "  clamp_u # Value-less fields\n"
    "  clamp_v\n"
    "}\n"
    "\n"
    "# And this is a nameless root node (requires SP_NAMELESS_ROOT_NODES):\n"
    "{\n"
    "  reason_for_using none\n"
    "}\n"
    "\n"
    "# And you can just pop down stuff anywhere in the root, really\n"
    "fullscreen\n"
    "fov 100\n"
    "\n"
    "# Semicolons can terminate a value and tell the parser to go onto a new field\n"
    "width 800; height 600\n";

#ifdef __BLOCKS__
  std::clog << "Using blocks implementation" << std::endl;
  sparse_parser_t parser(SP_TRIM_TRAILING_SPACES | SP_NAMELESS_ROOT_NODES,
                         ^(sparse_msg_t msg, const char *start, const char *end) {
    std::clog << "MSG " << msg << " [" << std::string(start, (size_t)(end - start)) << ']' << std::endl;
  });
#else
  std::clog << "Not using blocks implementation" << std::endl;
  sparse_parser_t parser(handler, NULL, SP_TRIM_TRAILING_SPACES | SP_NAMELESS_ROOT_NODES);
#endif

  parser.parse(test_source);

  parser.finish();

  return 0;
}