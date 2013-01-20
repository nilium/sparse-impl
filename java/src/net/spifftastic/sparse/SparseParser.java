package net.spifftastic.sparse;


import java.util.Arrays;


/**
 * Created with IntelliJ IDEA. User: ncower Date: 01/17/13 Time: 07:10 PM To change this template use File | Settings |
 * File Templates.
 */
public class SparseParser
{

  public interface ParseListener {
    void parsedName(String name);
    void parsedValue(String value);
    void parsedNodeOpening();
    void parsedNodeClosing();
  }

  private enum ParseMode {
    FindName,
    ReadName,
    FindValue,
    ReadValue,
    ReadComment
  }

  private final static int DEFAULT_BUFFER_CAPACITY = 64;

  // Options
  public final static int CONSUME_WHITESPACE = 0x1;
  public final static int TRIM_TRAILING_SPACES = 0x2;
  public final static int NAMELESS_ROOT_NODES = 0x4;
  public final static int DEFAULT_OPTIONS = TRIM_TRAILING_SPACES;

  // State constants
  private final boolean _consumeWhitespace;
  private final boolean _trimTrailingSpaces;
  private final boolean _allowNamelessRoots;
  private final ParseListener _listener;
  private final StringBuilder _buffer;

  // Mutable state
  private int _line;
  private int _column;
  private int _trailingSpaceCount;
  private int _depth;
  private boolean _inEscape;
  private char _last_char;
  private ParseMode _mode;
  private char[] _charBuffer;
  private boolean _finished;

  private static char[] ensureCharBufferSized(final char[] buffer, final int needed_size)
  {
    if (needed_size <= buffer.length) {
      return buffer;
    } else {
      final int doubledSize;
      final int newSize;

      doubledSize = buffer.length * 2;
      newSize = doubledSize >= needed_size
                ? doubledSize
                : needed_size;

      return Arrays.copyOf(buffer, newSize);
    }
  }

  private static boolean checkFlag(final int flags, final int flagMask)
  {
    return (flags & flagMask) == flagMask;
  }

  {
    _buffer = new StringBuilder(DEFAULT_BUFFER_CAPACITY);
    _trailingSpaceCount = 0;
    _depth = 0;
    _inEscape = false;
    _last_char = '\0';
    _charBuffer = new char[DEFAULT_BUFFER_CAPACITY];
    _line = 1;
    _column = 1;
    _finished = false;
    _mode = ParseMode.FindName;
  }

  public SparseParser(final int options, final ParseListener listener)
  {
    _listener = listener;

    _consumeWhitespace = checkFlag(options, CONSUME_WHITESPACE);
    _trimTrailingSpaces = checkFlag(options, TRIM_TRAILING_SPACES);
    _allowNamelessRoots = checkFlag(options, NAMELESS_ROOT_NODES);

    System.out.println("Flags: " + _consumeWhitespace + " " + _trimTrailingSpaces + " " + _allowNamelessRoots);
  }

  public SparseParser(final ParseListener listener)
  {
    this(DEFAULT_OPTIONS, listener);
  }

  public void finish() throws RuntimeException
  {
    validate();

    switch (_mode) {
    case ReadName: parsedName(grabAndResetBuffer());
    case FindValue: parsedValue(""); break;
    case ReadValue: parsedValue(grabAndResetBuffer()); break;
    default: break;
    }

    if (_depth != 0) {
      throw new RuntimeException(lineColumnPrefix() + " Unexpected end of document - expected } to match unclosed {.");
    }

    Arrays.fill(_charBuffer, '\0');
    _buffer.setLength(0);

    _finished = true;
  }


  private void validate() throws RuntimeException
  {
    if (_finished)
      throw new RuntimeException("Attempt to use finished SparseParser not permitted.");
  }


  public void parse(final String partial_source) throws RuntimeException
  {
    validate();

    final int source_length = partial_source.length();
    final char[] chars = ensureCharBufferSized(_charBuffer, source_length);
    partial_source.getChars(0, source_length, chars, 0);

    /* Maintain local copies of state just so there's not a ton of _s littering the place */
    boolean inEscape = _inEscape;
    ParseMode mode = _mode;
    char lastChar = _last_char;
    int depth = _depth;
    int line = _line;
    int column = _column;

    int chars_index = 0;

    for (; chars_index < source_length; ++chars_index) {
      final char currentChar = chars[chars_index];

      if (currentChar == '\n') {
        ++line;
        column = 0;
      }

      if (mode == ParseMode.ReadComment) {
        if (currentChar == '\n') {
          mode = ParseMode.FindName;
        }
      } else if (inEscape) {
        switch (currentChar) {
        case 'n': bufferCharacter('\n', true);     break;
        case 'r': bufferCharacter('\r', true);     break;
        case 'a': bufferCharacter('\u0007', true); break;
        case 'b': bufferCharacter('\b', true);     break;
        case 'f': bufferCharacter('\f', true);     break;
        case 't': bufferCharacter('\t', true);     break;
        case '0': bufferCharacter('\0', true);     break;
        default:  bufferCharacter(currentChar, true); break;
        }
        inEscape = false;
      } else {
        switch (currentChar) {
        case ' ':
        case '\t':
          if ((_consumeWhitespace && lastChar == currentChar) || mode == ParseMode.FindName || mode == ParseMode.FindValue) {
            break; // nop
          } else if (mode == ParseMode.ReadName) {
            parsedName(grabAndResetBuffer());
            mode = ParseMode.FindValue;
            break;
          }
          bufferCharacter(currentChar, inEscape);
          break;

        case '{':
          switch (mode) {
          case ReadName:
            parsedName(grabAndResetBuffer());
          case FindValue:
            ++depth;
            parsedNodeOpening();
            break;
          case ReadValue:
            parsedValue(grabAndResetBuffer());
          case FindName:
            if (depth == 0 && _allowNamelessRoots) {
              ++depth;
              parsedName("");
              parsedNodeOpening();
              break;
            }
          default:
            throw new RuntimeException(lineColumnPrefix(line, column) + " Invalid character '{' - expected name.");
          }

          mode = ParseMode.FindName;
          break;

        case '}':
          switch (mode) {
          case ReadName: parsedName(grabAndResetBuffer());
          case FindValue: parsedValue(""); break;
          case ReadValue: parsedValue(grabAndResetBuffer()); break;
          default: break;
          }

          if (depth == 0)
            throw new RuntimeException(lineColumnPrefix(line, column) + " Unexpected } - no matching {.");

          --depth;
          mode = ParseMode.FindName;

          parsedNodeClosing();
          break;

        case '\n':
          ++line;
          column = 0;
        case '#':
        case ';':
          switch (mode) {
          case ReadName: parsedName(grabAndResetBuffer());
          case FindValue: parsedValue(""); break;
          case ReadValue: parsedValue(grabAndResetBuffer()); break;
          }
          mode = currentChar == '#' ? ParseMode.ReadComment : ParseMode.FindName;
          break;

        case '\\':
          inEscape = true;
          break;

        default:
          if (mode == ParseMode.FindName)
            mode = ParseMode.ReadName;
          else if (mode == ParseMode.FindValue)
            mode = ParseMode.ReadValue;

          bufferCharacter(currentChar, false);
          break;
        }

      }

      lastChar = currentChar;
      ++column;
    }

    _inEscape = inEscape;
    _last_char = lastChar;
    _mode = mode;
    _depth = depth;
    _line = line;
    _column = column;
  }

  private String lineColumnPrefix()
  {
    return lineColumnPrefix(_line, _column);
  }

  private String lineColumnPrefix(final int line, final int column)
  {
    return "[" + line + ":" + column + "]";
  }

  private void bufferCharacter(final char character, final boolean escaped)
  {
    if (character != ' ' || escaped) {
      _trailingSpaceCount = 0;
    } else if (character == ' ' && _trimTrailingSpaces) {
      ++_trailingSpaceCount;
    }

    _buffer.append(character);
  }

  private String grabAndResetBuffer()
  {
    if (_trimTrailingSpaces && _trailingSpaceCount > 0) {
      _buffer.setLength(_buffer.length() - _trailingSpaceCount);
    }

    final String ref = _buffer.toString();
    _buffer.setLength(0);
    _trailingSpaceCount = 0;
    return ref;
  }


  protected void parsedName(final String name)
  {
    if (_listener != null) {
      _listener.parsedName(name);
    }
  }

  protected void parsedValue(final String value)
  {
    if (_listener != null) {
      _listener.parsedValue(value);
    }
  }

  protected void parsedNodeOpening()
  {
    if (_listener != null) {
      _listener.parsedNodeOpening();
    }
  }

  protected void parsedNodeClosing()
  {
    if (_listener != null) {
      _listener.parsedNodeClosing();
    }
  }

}
