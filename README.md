Sparse
======

Sparse is a sort of generic thing for defining lists of attributes. I designed
it ages ago (relative to my age at the moment) and it was always sort of an
oddity. It was made to be similar to the Quake 3 engine's shaders, so you'd
provide a name to start and some attributes and stages and so on and that would
define an element. It also ended up being similar to JSON, though at the time
I had no idea JSON even existed (and it's frankly better in all concievable
ways).

The original implementation was done in Blitz3D, which I never released because
it was an enormous piece of crap. I later implemented it in BlitzMax, where it
got a bit of use in my engine at the time because I needed something to define
materials again and it fit the bill since I'm a gigantic Quake 3-tech fanboy.
The C implementation is absurd and kind of pointless because I'm not sure I'll
ever use it. In other words, I was bored and someone reminded me of the old
Sparse code I wrote, so I ported it. Their mistake.

Anyway, Sparse is fairly simple and looks like this:

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

So, it's simple and fairly readable. The barebones API documentation follows.

API Reference
=============

All functions in Sparse are thread safe with the understanding that you should
never call them for the same state object from multiple threads. So, a state
object should not leave the thread it's in unless you're transferring its
ownership to something else. Basically, it's as thread-safe as OpenGL is. The
upside to this is that you can parse multiple Sparse documents in different
threads simply by creating new states.

In addition, you can re-use states. Unlike XML or JSON, you can have multiple
root nodes[^1] in Sparse, so concatenating documents is fine provided you parse
the documents completely and in order, otherwise things may get weird. You will
probably want a newline separating each document, however, just on the off
chance that you ended the previous document with a comment (which will eat the
first line of any following documents if it didn't end in a newline).


[^1]: This could be construed as a benefit, but it actually makes parsing
documents more complicated, so it's rarely useful.



### Errors

All functions in Sparse return error values. The error values are as follows:

* `SP_NO_ERROR`  
    No error occurred. Sparse did its job just fine and you should pat it on the
    head.
* `SP_ERROR_NO_MEM`  
    Sparse failed to allocate or reallocate (when expanding) its buffer. In most
    cases, you should never receive this error unless something is going
    extremely wrong in your code or you're running on a system from the '80s.
* `SP_ERROR_INVALID_CHAR`  
    The catch-all error. Sparse encountered a character that does not belong.
    The callback, if provided, will be passed the location of the character
    in the source string.
* `SP_ERROR_INCOMPLETE_DOCUMENT`  
    The document being parsed was incomplete at the time `sparse_end` was
    called. This typically means you left a node open somewhere.

There aren't a lot of errors because most Sparse documents are correct even when
they're incorrect. In other words, it's a _very_ dumb format.



--------------------------------------------------------------------------------

    sparse_error_t
    sparse_begin(sparse_state_t *state,
                 size_t initial_buffer_capacity,
                 sparse_options_t options,
                 sparse_fn_t callback);

Initializes a Sparse `state` object with the given parameters.

`initial_buffer_capacity` is exactly what it says on the tin -- if you pass zero
in for it, Sparse will use its default buffer capacity (128 bytes) which should
be sufficient for most purposes.

`options` are the parsing options you can use with Sparse. These are documented
below.

`callback` is the Sparse callback function, defined as:  
`typedef void (*sparse_fn_t)(sparse_msg_t msg, const char *begin, const char *end)`  
It takes a message and the begin- and end-point of a string. The strings may be
empty. You should never retain a pointer to the strings passed to this callback.
The strings are not guruanteed to be null-terminated and you should assume they
aren't. In certain cases, they may contain null characters as well -- they are
not the string's terminator, so just treat them like any other character in the
string.

You may optionally pass a NULL callback. There's not much reason for doing so,
but it's entirely possible and - if nothing else - it will sort of validate
your Sparse document. However, chances are you have no reason to validate a
Sparse document because most of it will be accidentally correct anyway. There
are only a few ways to make the parser spit out an error, and they're just to
prevent really odd cases (like nameless child nodes or incomplete documents or
nameless root nodes by default [this one is optional]).


### Options

* `SP_CONSUME_WHITESPACE`  
    Consumes whitespace so only one whitespace will ever show up in a value.
* `SP_TRIM_TRAILING_SPACES`  
    Trim trailing whitespace from the end of values.
* `SP_NAMELESS_ROOT_NODES`  
    Allow root nodes to be nameless (callback will be given NULL strings for a
    root node's name).


### Callback Messages

* `SP_ERROR`  
    Received when an error occurs. In most cases, this will point to a single
    character in the original string (helping you identify where the error
    occurred). If the document was incomplete or a buffer couldn't be allocated,
    then it will be an error string for those cases.
* `SP_BEGIN_NODE`  
    Received when a node is opened (a '{'). This is always preceeded by an
    `SP_NAME` message. Every `SP_BEGIN_NODE` message is eventually followed by
    an `SP_END_NODE` message. The string is the character it occurred on in the
    source string.
* `SP_END_NODE`  
    Received when a node is closed (a '}'). The string is the character it
    occurred on in the source string.
* `SP_NAME`  
    Received at the beginning of a field. The string for this message is the
    name of the field. If a state is initialized with the
    `SP_NAMELESS_ROOT_NODES` option, then the string may be empty (so the begin-
    and end-points of the string passed to the callback will be equal and point
    to an empty string).
* `SP_VALUE`  
    The string value of a field. This is always preceeded by an `SP_NAME`
    message. The string points to the field's value, of course. This value may
    point to an empty string if no value is provided for a field.



--------------------------------------------------------------------------------

    sparse_error_t
    sparse_end(sparse_state_t *state);

Closes out the document. This will finish off any parsing that was incomplete,
assuming the document is whole. This releases any resources used by Sparse and
zeroes the state object. If the document was not whole, it will return an error.

You must call this in order to deallocate any resources Sparse is using for a
given state.



--------------------------------------------------------------------------------

    sparse_error_t
    sparse_run(sparse_state_t *state,
               const char *const src_begin,
               const char *src_end);

Runs the Sparse parser on a string with the given state. This does not have to
be a complete document, but you eventually have to provide the complete document
through multiple calls to `sparse_run`. So, if you stream in a chunk of a Sparse
document, you can parse it while streaming in more of it. The chances of having
a Sparse document large enough to justify this is low, but it's something you
can do.

You may optionally pass NULL to `src_end` and it will try to get the end-point
of the string using strlen. If you know the end-point, it's better to pass it
to `sparse_run`.
