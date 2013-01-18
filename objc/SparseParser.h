#ifndef __CTM_SPARSEPARSER_H__
#define __CTM_SPARSEPARSER_H__

#import <Cocoa/Cocoa.h>
#import <sparse.h>

@interface SPSparseParser : NSObject

#ifdef __BLOCKS__
- (id)initUsingBlock:(sparse_block_t)block;
- (id)initWithOptions:(sparse_options_t)options usingBlock:(sparse_block_t)block;
- (id)initWithOptions:(sparse_options_t)options capacity:(size_t)capacity usingBlock:(sparse_block_t)block;
#endif

- (id)initUsingCallback:(sparse_fn_t)callback context:(void *)ctx;
- (id)initWithOptions:(sparse_options_t)options usingCallback:(sparse_fn_t)callback context:(void *)ctx;
- (id)initWithOptions:(sparse_options_t)options capacity:(size_t)capacity usingCallback:(sparse_fn_t)callback context:(void *)ctx;

- (sparse_error_t)parseUTF8String:(const char *)string;
- (sparse_error_t)parseUTF8StringFrom:(const char *)start to:(const char *)end;
- (sparse_error_t)parseString:(NSString *)string;
- (sparse_error_t)parseData:(NSData *)data; // Expects data to be a UTF-8 encoded array of characters

- (sparse_error_t)finish;

@property (readonly) BOOL finished;

@end

#endif /* end of include guard: __CTM_SPARSEPARSER_H__ */
