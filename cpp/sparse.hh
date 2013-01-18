#ifndef __CMT_SPARSE_HH__
#define __CMT_SPARSE_HH__

#include "sparse.h"
#include <stdexcept>
#include <string>

/* Exceptions because those are always fun */
class sparse_exception_t : public std::runtime_error {
public:
  sparse_exception_t(const std::string &what);
  virtual ~sparse_exception_t() throw();
};

class sparse_invalid_char_error_t : public sparse_exception_t {
private:
  const char *err_begin;
  const char *err_end;
  char *err_str;
public:
  sparse_invalid_char_error_t(const std::string &what, const char *begin, const char *end);
  virtual ~sparse_invalid_char_error_t() throw();
  virtual const char *error_begin() const;
  virtual const char *error_end() const;
  virtual std::string error_string() const;
};

class sparse_no_mem_error_t : public sparse_exception_t {
public:
  sparse_no_mem_error_t(const std::string &what);
  virtual ~sparse_no_mem_error_t() throw();
};

class sparse_incomplete_document_error_t : public sparse_exception_t {
public:
  sparse_incomplete_document_error_t(const std::string &what);
  virtual ~sparse_incomplete_document_error_t() throw();
};


/* Parser */
class sparse_parser_t : protected sparse_state_t
{
private:
  bool finished;

public:
  // Can throw exceptions if buffer allocation fails, but that's it.
  sparse_parser_t(sparse_fn_t callback, void *context = NULL,
                  int options = SP_DEFAULT_OPTIONS,
                  size_t initial_buffer_capacity = 0) throw(sparse_no_mem_error_t);

#ifdef __BLOCKS__
  sparse_parser_t(sparse_block_t block) throw(sparse_no_mem_error_t);
  sparse_parser_t(int options, sparse_block_t block) throw(sparse_no_mem_error_t);
  sparse_parser_t(int options, size_t initial_buffer_capacity, sparse_block_t block) throw(sparse_no_mem_error_t);
#endif
  // In case of error, will write a message to cerr.
  virtual ~sparse_parser_t();

  virtual sparse_error_t parse(const std::string &src) throw(sparse_exception_t);
  virtual sparse_error_t parse(const char *const src_begin, const char *const src_end) throw(sparse_exception_t);

  // Can be called explicitly or will be called by the destructor.
  virtual sparse_error_t finish(void) throw(sparse_exception_t);

private:
  virtual std::string state_error_string(void) const;
};

#endif /* end __SPARSE_HH__ include guard */
