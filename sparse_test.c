#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sparse.h"

static void check_sparse_result(sparse_error_t error);
static void sparse_handle(sparse_msg_t msg, const char *begin, const char *end, void *context);

static void check_sparse_result(sparse_error_t error)
{
  /* Check for errors. There aren't very many. A lot of stuff in Sparse is
     accidentally valid due to how little it cares about things. */
  switch(error) {
  case SP_ERROR_NO_MEM:
    fprintf(stderr, "Sparse couldn't allocate its internal buffer.\n");
    break;
  case SP_ERROR_INVALID_CHAR:
    fprintf(stderr, "Sparse encountered an invalid character (passed to callback).\n");
    break;
  case SP_ERROR_INCOMPLETE_DOCUMENT:
    fprintf(stderr, "The document provided to Sparse was incomplete when sparse_end was called.\n");
    break;
  case SP_NO_ERROR:
    break;
  }

  if (error != SP_NO_ERROR)
    exit(1);
}

static void sparse_handle(sparse_msg_t msg, const char *begin, const char *end, void *context)
{
  (void)context;

  char *tstring;
  size_t string_len = (begin == NULL ? 0 : (size_t)(end - begin));

  /* Copy the string passed to sparse_handle (always copy it if possible). */
  if (string_len > 0) {
    tstring = calloc(string_len, sizeof(*tstring));
    strncpy(tstring, begin, string_len);
  } else {
    tstring = "EMPTY";
  }

  /* And do something with it - in this case, print it. */
  printf("MSG %d -> [%s]\n", (int)msg, tstring);

  if (string_len > 0)
    free(tstring);
}

int main(int argc, char const *argv[])
{
  (void)argc; (void)argv;

  static const char *test_string =
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

  sparse_state_t state;
  sparse_options_t options = SP_CONSUME_WHITESPACE | SP_TRIM_TRAILING_SPACES | SP_NAMELESS_ROOT_NODES;

  check_sparse_result(sparse_begin(&state, 0, options, sparse_handle, NULL));
  check_sparse_result(sparse_run(&state, test_string, test_string + 10));
  check_sparse_result(sparse_run(&state, test_string + 10, NULL));
  check_sparse_result(sparse_end(&state));
  return 0;
}
