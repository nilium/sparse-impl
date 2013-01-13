#ifndef __CMT_SPARSE_H__
#define __CMT_SPARSE_H__

#include <stddef.h>

#define SP_DEFAULT_BUFFER_CAPACITY (128)

typedef enum {
  SP_FIND_NAME,
  SP_READ_NAME,
  SP_FIND_OPENING,
  SP_FIND_VALUE,
  SP_READ_VALUE,
  SP_READ_COMMENT
} sparse_mode_t;

typedef enum {
  SP_NO_ERROR =                  0,
  SP_ERROR_NO_MEM =              1,
  SP_ERROR_INVALID_CHAR =        2,
  SP_ERROR_INCOMPLETE_DOCUMENT = 3
} sparse_error_t;

typedef enum {
  SP_ERROR =      -1,       // Errors
  SP_BEGIN_NODE =  1,       // {
  SP_END_NODE =    2,       // }
  SP_NAME =        3,       // <FieldName> ...
  SP_VALUE =       4        // FieldName <FieldValue>
} sparse_msg_t;

typedef enum {
  SP_CONSUME_WHITESPACE =   0x1,  // Consumes whitespace so only one whitespace will ever show up in a value.
  SP_TRIM_TRAILING_SPACES = 0x2,  // Trim trailing whitespace from the end of values.
  SP_NAMELESS_ROOT_NODES =  0x4   // Allow root nodes to be nameless (callback will be given NULL strings for a root node's name).
} sparse_options_t;

#ifdef __BLOCKS__
typedef void (^sparse_block_t)(sparse_msg_t msg, const char *begin, const char *end);
#endif
typedef void (*sparse_fn_t)(sparse_msg_t msg, const char *begin, const char *end, void *context);

typedef struct s_sparse_state {
  char *buffer;

  size_t buffer_capacity;
  size_t buffer_size;
  size_t num_spaces_trailing;
  size_t depth;

  sparse_options_t options;
  sparse_mode_t mode;
  sparse_mode_t last_mode;

  int in_escape;
  int last_char;

  void *context;

  sparse_fn_t callback;
#ifdef __BLOCKS__
  sparse_block_t block;
#endif
} sparse_state_t;

sparse_error_t sparse_begin(sparse_state_t *state, size_t initial_buffer_capacity, sparse_options_t options, sparse_fn_t callback, void *context);
#ifdef __BLOCKS__
sparse_error_t sparse_begin_using_block(sparse_state_t *state, size_t initial_buffer_capacity, sparse_options_t options, sparse_block_t block);
#endif
sparse_error_t sparse_end(sparse_state_t *state);
sparse_error_t sparse_run(sparse_state_t *state, const char *const src_begin, const char *src_end);

#endif /* end __CMT_SPARSE_H__ include guard */
