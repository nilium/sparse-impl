#include "sparse.h"
#include <stdlib.h>
#include <string.h>
#ifdef __BLOCKS__
#include <block.h>
#endif

static const char *sp_empty_str = "";
static const char *sp_errstr_no_mem = "Could not allocate memory for sparse buffer.";
static const char *sp_errstr_incomplete_doc = "Document is incomplete.";

#ifdef __BLOCKS__
#define SP_HAS_CALLBACK() (callback != NULL || block != NULL)
#define SP_SEND_MSG(MSG, BEGIN, END) {                  \
    if (block != NULL) {                                \
      block((MSG), (BEGIN), (END));                     \
    } else if (callback != NULL) {                      \
      callback((MSG), (BEGIN), (END), context);         \
    }                                                   \
  }
#else
#define SP_HAS_CALLBACK() (callback != NULL)
#define SP_SEND_MSG(MSG, BEGIN, END)  {                 \
    if (callback != NULL) {                             \
      callback((MSG), (BEGIN), (END), context);         \
    }                                                   \
  }
#endif

#define SP_CHECK_FLAG(FLAGS, FLAG) (((FLAGS)&(FLAG)) == (FLAG))
#define SP_ENSURE_BUFFER_SIZED(BUFFER, CURRENT, NEEDED) \
  ((CURRENT) < (NEEDED)                                 \
   ? (CURRENT = (CURRENT) * 2),                         \
     (((CURRENT) < (NEEDED)                             \
      ? (CURRENT = (NEEDED))                            \
      : 0)),                                            \
     (BUFFER = realloc(BUFFER, CURRENT))                \
   : BUFFER)
#define SP_RETURN_ERROR(ERRNAME, START, END) {          \
    const char *sp_err_begin = (START);                 \
    const char *sp_err_end = (END);                     \
    if (SP_HAS_CALLBACK()) {                            \
      SP_SEND_MSG(SP_ERROR, sp_err_begin, sp_err_end);  \
    }                                                   \
    error = (ERRNAME);                                  \
    state->error_begin = sp_err_begin;                  \
    state->error_end = sp_err_end;                      \
    goto sparse_exit;                                   \
  }
#define SP_ERRSTR_END(NAME) (NAME) + strlen((NAME))

sparse_error_t sparse_begin(sparse_state_t *state, size_t initial_buffer_capacity, sparse_options_t options, sparse_fn_t callback, void *context)
{
  char *buffer = NULL;

  memset(state, 0, sizeof(*state));

  state->options = options;
  state->mode = SP_FIND_NAME;
  state->last_mode = SP_FIND_NAME;
  state->callback = callback;
  state->context = context;

  if (initial_buffer_capacity == 0)
    initial_buffer_capacity = SP_DEFAULT_BUFFER_CAPACITY;

  buffer = calloc(initial_buffer_capacity, sizeof(*buffer));

  if (buffer == NULL) {
    state->error_begin = sp_errstr_no_mem;
    state->error_end = SP_ERRSTR_END(sp_errstr_no_mem);
    return SP_ERROR_NO_MEM;
  }

  state->buffer = buffer;
  state->buffer_capacity = initial_buffer_capacity;

  return SP_NO_ERROR;
}

#ifdef __BLOCKS__
sparse_error_t sparse_begin_using_block(sparse_state_t *state, size_t initial_buffer_capacity, sparse_options_t options, sparse_block_t block)
{
  const sparse_error_t error = sparse_begin(state, initial_buffer_capacity, options, NULL, NULL);
  /*
    Could go back and reimplement block support by providing a callback that
    recognizes its context pointer as a block, but then I'm not sure I can
    guarantee a block will fit in the space of a pointer at all times. Kind of
    like passing function pointers as void* - it's probably not really a good
    idea.
  */
  if (error == SP_NO_ERROR)
    state->block = Block_copy(block);
  return error;
}
#endif

sparse_error_t sparse_end(sparse_state_t *state)
{
  sparse_error_t error = SP_NO_ERROR;

  sparse_fn_t callback = state->callback;
  void *context = state->context;
#ifdef __BLOCKS__
  sparse_block_t block = state->block;
#endif

  if (state->mode == SP_READ_NAME) {
    SP_SEND_MSG(SP_NAME, state->buffer, state->buffer + state->buffer_size - state->num_spaces_trailing);
    SP_SEND_MSG(SP_VALUE, sp_empty_str, sp_empty_str);
  } else if (state->mode == SP_READ_VALUE) {
    SP_SEND_MSG(SP_VALUE, state->buffer, state->buffer + state->buffer_size - state->num_spaces_trailing);
  }

  if (state->depth != 0) {
    error = SP_ERROR_INCOMPLETE_DOCUMENT;
    SP_SEND_MSG(SP_ERROR, sp_errstr_incomplete_doc, SP_ERRSTR_END(sp_errstr_incomplete_doc));
    state->error_begin = sp_errstr_incomplete_doc;
    state->error_end = SP_ERRSTR_END(sp_errstr_incomplete_doc);
  }

#ifdef __BLOCKS__
  if (state->block != NULL)
    Block_release(state->block);
#endif

  if (state->buffer != NULL)
    free(state->buffer);

  if (error != SP_NO_ERROR)
    memset(state, 0, sizeof(*state));

  return error;
}

sparse_error_t sparse_run(sparse_state_t *state, const char *const src_begin, const char *src_end)
{
  sparse_error_t error = SP_NO_ERROR;

  const sparse_options_t options = state->options;
  const int consume_whitespace = SP_CHECK_FLAG(options, SP_CONSUME_WHITESPACE);
  const int trim_spaces = SP_CHECK_FLAG(options, SP_TRIM_TRAILING_SPACES);
  const int nameless_roots = SP_CHECK_FLAG(options, SP_NAMELESS_ROOT_NODES);

  const char *src_iter = src_begin;

  size_t buffer_capacity = state->buffer_capacity;
  size_t buffer_size = state->buffer_size;
  size_t num_spaces_trailing = state->num_spaces_trailing;
  char *buffer = state->buffer;

  int current_char = 0;
  int last_char = state->last_char;
  int in_escape = state->in_escape;

  sparse_mode_t mode = state->mode;
  sparse_mode_t last_mode = state->last_mode;

  sparse_fn_t callback = state->callback;
  void *context = state->context;
#if __BLOCKS__
  sparse_block_t block = state->block;
#endif

  size_t depth = state->depth;

  /* If no end provided, try to guess where the end is */
  if (src_end == NULL)
    src_end = src_begin + strlen(src_begin);

  for (; src_iter != src_end; ++src_iter) {
    last_char = current_char;
    current_char = (int)*src_iter;

    if (mode == SP_READ_COMMENT) {
      if (current_char == '\n')
        mode = last_mode;

      continue;
    } else if (in_escape) {
      switch (current_char) {
      case 'n': current_char = '\n'; break;
      case 'r': current_char = '\r'; break;
      case 'a': current_char = '\a'; break;
      case 'b': current_char = '\b'; break;
      case 'f': current_char = '\f'; break;
      case 't': current_char = '\t'; break;
      default: break;
      }
      in_escape = 0;
      goto sparse_buffer_char; // skip space trimming (space was escaped)
    }

    switch (current_char) {
    case ' ':
    case '\t':
      if ((consume_whitespace && (last_char == current_char)) || mode == SP_FIND_NAME || mode == SP_FIND_VALUE) {
        continue;
      } else if (mode == SP_READ_NAME) {
        mode = SP_FIND_VALUE;

        SP_SEND_MSG(SP_NAME, buffer, buffer + buffer_size - num_spaces_trailing);
        buffer_size = 0;
        continue;
      }
      goto sparse_buffer_char_trim_spaces;

    case '#':
      if (mode == SP_READ_VALUE) {
        mode = SP_FIND_NAME;

        SP_SEND_MSG(SP_VALUE, buffer, buffer + buffer_size - num_spaces_trailing);
        buffer_size = 0;
      } else if (mode == SP_READ_NAME) {
        mode = SP_FIND_NAME;

        SP_SEND_MSG(SP_NAME, buffer, buffer + buffer_size - num_spaces_trailing);
        SP_SEND_MSG(SP_VALUE, sp_empty_str, sp_empty_str);
        buffer_size = 0;
      } else if (mode == SP_FIND_VALUE) {
        mode = SP_FIND_NAME;

        SP_SEND_MSG(SP_VALUE, sp_empty_str, sp_empty_str);
      }

      last_mode = mode;
      mode = SP_READ_COMMENT;
      break;

    case '\n':
      if (mode == SP_READ_NAME) {
        mode = SP_FIND_NAME;

        SP_SEND_MSG(SP_NAME, buffer, buffer + buffer_size - num_spaces_trailing);
        SP_SEND_MSG(SP_VALUE, sp_empty_str, sp_empty_str);
        buffer_size = 0;
      } else if (mode == SP_READ_VALUE) {
        mode = SP_FIND_NAME;

        SP_SEND_MSG(SP_VALUE, buffer, buffer + buffer_size - num_spaces_trailing);
        buffer_size = 0;
      }
      break;

    case '{': // open node
      if (mode == SP_FIND_VALUE) {
        ++depth;
        mode = SP_FIND_NAME;

        SP_SEND_MSG(SP_BEGIN_NODE, src_iter, src_iter + 1);

        continue;
      } else if (mode == SP_FIND_NAME && depth == 0 && nameless_roots) {
        ++depth;
        SP_SEND_MSG(SP_NAME, sp_empty_str, sp_empty_str);
        SP_SEND_MSG(SP_BEGIN_NODE, src_iter, src_iter + 1);

        continue;
      }

      SP_RETURN_ERROR(SP_ERROR_INVALID_CHAR, src_iter, src_iter + 1);
      break;

    case '}':
      if (mode == SP_READ_VALUE) {
        SP_SEND_MSG(SP_VALUE, buffer, buffer + buffer_size - num_spaces_trailing);
        buffer_size = 0;
      } else if (mode != SP_FIND_NAME) {
        SP_RETURN_ERROR(SP_ERROR_INVALID_CHAR, src_iter, src_iter + 1);
      }

      if (depth == 0)
        SP_RETURN_ERROR(SP_ERROR_INVALID_CHAR, src_iter, src_iter + 1);

      --depth;
      mode = SP_FIND_NAME;

      SP_SEND_MSG(SP_END_NODE, src_iter, src_iter + 1);

      break;

    case ';':
      switch (mode) {
      case SP_READ_VALUE:
        SP_SEND_MSG(SP_VALUE, buffer, buffer + buffer_size - num_spaces_trailing);
        goto sparse_semicolon_reset;

      case SP_FIND_VALUE:
        SP_SEND_MSG(SP_VALUE, sp_empty_str, sp_empty_str);
        goto sparse_semicolon_reset;

      case SP_READ_NAME:
        SP_SEND_MSG(SP_NAME, buffer, buffer + buffer_size - num_spaces_trailing);
        SP_SEND_MSG(SP_VALUE, sp_empty_str, sp_empty_str);

        sparse_semicolon_reset:
        buffer_size = 0;
        mode = SP_FIND_NAME;

      default:
        // nop
        break;
      }
      break;

    case '\\':  // escape
      in_escape = 1;
      break;

    default:
      sparse_buffer_char_trim_spaces:
      if (trim_spaces) {
        if (current_char == ' ') {
          ++num_spaces_trailing;
        } else {
          sparse_buffer_char:
          num_spaces_trailing = 0;
        }
      }

      if (mode == SP_FIND_NAME)
        mode = SP_READ_NAME;
      else if (mode == SP_FIND_VALUE)
        mode = SP_READ_VALUE;

      if (!SP_HAS_CALLBACK())
        break;

      buffer = SP_ENSURE_BUFFER_SIZED(buffer, buffer_capacity, buffer_size + 1);

      if (buffer == NULL)
        SP_RETURN_ERROR(SP_ERROR_NO_MEM, sp_errstr_no_mem, SP_ERRSTR_END(sp_errstr_no_mem));

      buffer[buffer_size++] = (char)current_char;
      break;
    }
  }

  sparse_exit:

  state->buffer = buffer;
  state->buffer_capacity = buffer_capacity;
  state->buffer_size = buffer_size;
  state->num_spaces_trailing = num_spaces_trailing;
  state->depth = depth;
  state->mode = mode;
  state->last_mode = last_mode;
  state->in_escape = in_escape;
  state->last_char = last_char;

  return error;
}

#undef SP_CHECK_FLAG
#undef SP_ENSURE_BUFFER_SIZED
#undef SP_RETURN_ERROR
