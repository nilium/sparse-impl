#import "SparseParser.h"

/* Note: requires ARC */

@interface SPSparseParser ()

@property (readwrite) BOOL finished;

@end

@implementation SPSparseParser {
  sparse_state_t _state;
  BOOL _finished;
}

@synthesize finished = _finished;

- (id)init
{
  return [self initWithOptions:SP_DEFAULT_OPTIONS capacity:0 usingCallback:NULL context:NULL];
}

#ifdef __BLOCKS__

- (id)initUsingBlock:(sparse_block_t)block
{
  return [self initWithOptions:SP_DEFAULT_OPTIONS capacity:0 usingBlock:block];
}

- (id)initWithOptions:(sparse_options_t)options usingBlock:(sparse_block_t)block
{
  return [self initWithOptions:options capacity:0 usingBlock:block];
}

- (id)initWithOptions:(sparse_options_t)options capacity:(size_t)capacity usingBlock:(sparse_block_t)block
{
  if ((self = [super init])) {
    if (sparse_begin_using_block(&_state, capacity, options, block)) {
      self = nil;
    }
  }
  return self;
}

#endif

- (id)initUsingCallback:(sparse_fn_t)callback context:(void *)ctx
{
  return [self initWithOptions:SP_DEFAULT_OPTIONS capacity:0 usingCallback:callback context:ctx];
}

- (id)initWithOptions:(sparse_options_t)options usingCallback:(sparse_fn_t)callback context:(void *)ctx
{
  return [self initWithOptions:options capacity:0 usingCallback:callback context:ctx];
}

- (id)initWithOptions:(sparse_options_t)options capacity:(size_t) capacity usingCallback:(sparse_fn_t)callback context:(void *)ctx
{
  if ((self = [super init])) {
    if (sparse_begin(&_state, capacity, options, callback, ctx)) {
      self = nil;
    }
  }
  return self;
}

- (sparse_error_t)parseUTF8String:(const char *)string
{
  if (_finished) return SP_ERROR_INCOMPLETE_DOCUMENT;
  return sparse_run(&_state, string, NULL);
}

- (sparse_error_t)parseUTF8StringFrom:(const char *)start to:(const char *)end
{
  if (_finished) return SP_ERROR_INCOMPLETE_DOCUMENT;
  return sparse_run(&_state, start, end);
}

- (sparse_error_t)parseString:(NSString *)string
{
  if (_finished) return SP_ERROR_INCOMPLETE_DOCUMENT;
  const char *utf8string = [string UTF8String];
  size_t length = (size_t)[string lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
  return sparse_run(&_state, utf8string, utf8string + length);
}

- (sparse_error_t)parseData:(NSData *)data
{
  if (_finished) return SP_ERROR_INCOMPLETE_DOCUMENT;

  size_t length = [data length];
  const char *utf8string = [data bytes];
  if (utf8string == nil) {
    return SP_NO_ERROR;
  }
  return sparse_run(&_state, utf8string, utf8string + length);
}

- (sparse_error_t)finish
{
  sparse_error_t result;

  if (_finished)
    return SP_ERROR_INCOMPLETE_DOCUMENT;

  result = sparse_end(&_state);
  self.finished = YES;

  return result;
}

- (void)dealloc
{
  if (!self.finished)
    [self finish];
}

@end
