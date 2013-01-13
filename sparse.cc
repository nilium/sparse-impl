#include "sparse.hh"
#include <iostream>

sparse_parser_t::sparse_parser_t(sparse_fn_t callback,
                                 void *context,
                                 int options,
                                 size_t initial_buffer_capacity) throw(sparse_no_mem_error_t)
: finished(false)
{
  const sparse_error_t error = sparse_begin(this, initial_buffer_capacity, (sparse_options_t)options, callback, context);
  if (error == SP_ERROR_NO_MEM)
    throw sparse_no_mem_error_t(state_error_string());
}

#ifdef __BLOCKS__
sparse_parser_t::sparse_parser_t(sparse_block_t block) throw(sparse_no_mem_error_t)
: finished(false)
{
  const sparse_error_t error = sparse_begin_using_block(this, 0, SP_DEFAULT_OPTIONS, block);
  if (error == SP_ERROR_NO_MEM)
    throw sparse_no_mem_error_t(state_error_string());
}

sparse_parser_t::sparse_parser_t(int options, sparse_block_t block) throw(sparse_no_mem_error_t)
: finished(false)
{
  const sparse_error_t error = sparse_begin_using_block(this, 0, (sparse_options_t)options, block);
  if (error == SP_ERROR_NO_MEM)
    throw sparse_no_mem_error_t(state_error_string());
}

sparse_parser_t::sparse_parser_t(int options,
                                 size_t initial_buffer_capacity,
                                 sparse_block_t block) throw(sparse_no_mem_error_t)
: finished(false)
{
  const sparse_error_t error = sparse_begin_using_block(this, initial_buffer_capacity, (sparse_options_t)options, block);
  if (error == SP_ERROR_NO_MEM)
    throw sparse_no_mem_error_t(state_error_string());
}
#endif

sparse_parser_t::~sparse_parser_t()
{
  if (!finished) {
    try {
      finish();
    } catch (sparse_exception_t &ex) {
      std::clog << "Error occurred closing Sparse parser on destruction:" << std::endl << ex.what() << std::endl;
    }
  }
}

sparse_error_t sparse_parser_t::parse(const std::string &src) throw(sparse_exception_t)
{
  const char *src_begin = src.c_str();
  const char *src_end = src_begin + src.size();
  sparse_error_t error = SP_NO_ERROR;
  try {
    error = parse(src_begin, src_end);
  } catch(std::runtime_error &) {
    throw;
  }
  return error;
}

sparse_error_t sparse_parser_t::parse(const char *const src_begin, const char *const src_end) throw(sparse_exception_t)
{
  sparse_error_t error = sparse_run(this, src_begin, src_end);
  switch (error) {
  case SP_NO_ERROR:
    return error;
  case SP_ERROR_NO_MEM:
    throw sparse_no_mem_error_t(state_error_string());
  case SP_ERROR_INVALID_CHAR:
    throw sparse_invalid_char_error_t("Invalid character encountered.", error_begin, error_end);
  default:
    throw sparse_exception_t(state_error_string());
  }
}

sparse_error_t sparse_parser_t::finish() throw(sparse_exception_t)
{
  if (finished)
    throw sparse_exception_t("Parser already finished.");

  const sparse_error_t error = sparse_end(this);
  if (error == SP_ERROR_INCOMPLETE_DOCUMENT)
    throw sparse_incomplete_document_error_t(state_error_string());

  finished = true;

  return error;
}

std::string sparse_parser_t::state_error_string(void) const
{
  const size_t count = (size_t)(error_end - error_begin);
  return count == 0 ? std::string() : std::string(error_begin, count);
}


/* Exceptions */

sparse_exception_t::sparse_exception_t(const std::string &what)
: std::runtime_error(what)
{
}

sparse_exception_t::~sparse_exception_t() throw()
{
}

sparse_invalid_char_error_t::sparse_invalid_char_error_t(const std::string &what, const char *begin, const char *end)
: sparse_exception_t(what), err_begin(begin), err_end(end)
{
  const size_t length = (size_t)(end - begin);
  err_str = new char[length + 1];
  memcpy(err_str, err_begin, length);
  err_str[length] = '\0';
}

sparse_invalid_char_error_t::~sparse_invalid_char_error_t() throw()
{
  delete [] err_str;
}

const char *sparse_invalid_char_error_t::error_begin() const
{
  return err_begin;
}

const char *sparse_invalid_char_error_t::error_end() const
{
  return err_end;
}

std::string sparse_invalid_char_error_t::error_string() const
{
  return err_str;
}

sparse_no_mem_error_t::sparse_no_mem_error_t(const std::string &what)
: sparse_exception_t(what)
{
}

sparse_no_mem_error_t::~sparse_no_mem_error_t() throw()
{
}

sparse_incomplete_document_error_t::sparse_incomplete_document_error_t(const std::string &what)
: sparse_exception_t(what)
{
}

sparse_incomplete_document_error_t::~sparse_incomplete_document_error_t() throw()
{
}
