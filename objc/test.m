#import <Cocoa/Cocoa.h>
#import "SparseParser.h"

static NSString *test_source = @
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

int main(int argc, char const *argv[])
{
  @autoreleasepool {
    sparse_block_t testBlock = ^(sparse_msg_t msg, const char *start, const char *end) {
    NSString *string = [[NSString alloc]
                        initWithBytesNoCopy:(void *)start length:(size_t)(end - start)
                        encoding:NSUTF8StringEncoding freeWhenDone:NO];
      NSLog(@"MSG %d -> [%@]", msg, string);
    };

    SPSparseParser *parser = [[SPSparseParser alloc]
                              initWithOptions:SP_TRIM_TRAILING_SPACES | SP_NAMELESS_ROOT_NODES
                              usingBlock:testBlock];

    [parser parseString:test_source];
    [parser finish];
  }
  return 0;
}