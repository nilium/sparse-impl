SuperStrict

Module cower.sparse

Import brl.LinkedList

Private

Function LineColString$(line%, column%)
  Return "[" + line + ":" + column + "]"
EndFunction

Function ExpandBufferToFit:Short[](buffer:Short[], newLength%)
  If buffer = Null
    Return New Short[newLength]
  EndIf

  If buffer.Length < newLength
    Local doubledLength% = buffer.Length * 2
    If doubledLength > newLength
      newLength = doubledLength
    EndIf
    buffer = New Short[newLength]
  EndIf

  Return buffer
EndFunction

' Buffer capacity
Const SP_DEFAULT_CAPACITY% = 64

' States
Const SP_FIND_NAME% = 0
Const SP_FIND_VALUE% = 1
Const SP_READ_NAME% = 2
Const SP_READ_VALUE% = 3
Const SP_READ_COMMENT% = 4

' Character constants
Const SP_NULL% = 0
Const SP_BELL% = 7
Const SP_BACKSPACE% = 8
Const SP_TAB% = 9
Const SP_FORMFEED% = 12
Const SP_RETURN% = 13
Const SP_NEWLINE% = 10
Const SP_SPACE% = 32
Const SP_HASH% = 35
Const SP_ZERO% = 48
Const SP_SEMICOLON% = 59
Const SP_BACKSLASH% = 92
Const SP_LOWER_A% = 97
Const SP_LOWER_B% = 98
Const SP_LOWER_F% = 102
Const SP_LOWER_N% = 110
Const SP_LOWER_R% = 114
Const SP_LOWER_T% = 116
Const SP_OPEN_BRACE% = 123
Const SP_CLOSE_BRACE% = 125

Public

' Parser Options
Const SP_TRIM_TRAILING_SPACES% = $1
Const SP_NAMELESS_ROOT_NODES% = $2
Const SP_CONSUME_WHITESPACE% = $4
Const SP_DEFAULT_OPTIONS% = SP_TRIM_TRAILING_SPACES

' Callback Messages
Const SP_NAME% = 1
Const SP_VALUE% = 2
Const SP_NODE_OPENING% = 3
Const SP_NODE_CLOSING% = 4

Type TSparseException
  Field line%, column%
  Field message$

  Method Init:TSparseException(message$, line% = -1, column% = -1)
    Self.message = message
    Self.line = line
    Self.column = column
    Return Self
  EndMethod

  Method ToString$()
    If line = -1
      Return message
    Else
      Return LineColString(line, column) + " " + message
    EndIf
  EndMethod
EndType

Type TSparseParser

  Field _finished%
  Field _trim_spaces%
  Field _nameless_roots%
  Field _consume_whitespace%
  Field _in_escape%
  Field _depth%
  Field _line%
  Field _column%
  Field _mode%
  Field _space_count%
  Field _last_char%
  Field _buffer_off%
  Field _buffer:Short[]
  Field _context:Object
  Field _callback%(msg%, content$, context:Object)
  Field _openings:TList

  Method Init:TSparseParser(callback%(msg%, content$, context:Object), ..
                            context:Object = Null, options% = SP_DEFAULT_OPTIONS)
    _finished = False
    _trim_spaces = (options & SP_TRIM_TRAILING_SPACES) = SP_TRIM_TRAILING_SPACES
    _nameless_roots = (options & SP_NAMELESS_ROOT_NODES) = SP_NAMELESS_ROOT_NODES
    _consume_whitespace = (options & SP_CONSUME_WHITESPACE) = SP_CONSUME_WHITESPACE
    _in_escape = False
    _depth = 0
    _line = 1
    _column = 1
    _mode = SP_FIND_NAME
    _space_count = 0
    _last_char = SP_NULL
    _buffer_off = 0
    If _buffer = Null
      _buffer = New Short[SP_DEFAULT_CAPACITY]
    EndIf
    _context = context
    _callback = callback
    If _openings = Null
      _openings = New TList
    Else
      _openings.Clear()
    EndIf
    Return Self
  EndMethod

  Method Parse(source$)
    If _finished
      Throw New TSparseException.Init("Attempt to continue parsing using finalized parser")
    EndIf

    Local index%

    For index = 0 Until source.Length
      Local char% = source[index]

      If _mode = SP_READ_COMMENT
        If char = SP_NEWLINE
          _mode = SP_FIND_NAME
        EndIf
      ElseIf _in_escape
        Select char
        Case SP_LOWER_N ; _BufferChar(SP_NEWLINE, True)
        Case SP_LOWER_R ; _BufferChar(SP_RETURN, True)
        Case SP_LOWER_F ; _BufferChar(SP_FORMFEED, True)
        Case SP_LOWER_B ; _BufferChar(SP_BACKSPACE, True)
        Case SP_LOWER_T ; _BufferChar(SP_TAB, True)
        Case SP_LOWER_A ; _BufferChar(SP_BELL, True)
        Case SP_ZERO    ; _BufferChar(SP_NULL, True)
        Default         ; _BufferChar(char, True)
        EndSelect
      Else
        Select char
        Case SP_SPACE, SP_TAB
          _ParseWhitespace(char)

        Case SP_NEWLINE, SP_SEMICOLON, SP_HASH
          _ParseTerminateField(True, (char = SP_HASH) * SP_READ_COMMENT)

        Case SP_OPEN_BRACE
          _ParseOpening()

        Case SP_CLOSE_BRACE
          _ParseClosing()

        Case SP_BACKSLASH
          _in_escape = True

        Default
          If _mode <= SP_FIND_VALUE
            _mode :+ 2
          EndIf
          _BufferChar(char, False)
        EndSelect
      EndIf

      _last_char = char

      If char = SP_NEWLINE
        _line :+ 1
        _column = 1
      Else
        _column :+ 1
      EndIf
    Next
  EndMethod

  Method Finalize()
    If _finished
      Throw New TSparseException.Init("Attempt to finalize already-finalized parser")
    EndIf

    _ParseTerminateField(True, SP_FIND_NAME)

    If _depth > 0
      Throw _openings.Last()
    EndIf

    _finished = True
  EndMethod

  Method ParsedName(name$)
    If _callback
      _callback(SP_NAME, name, _context)
    EndIf
  EndMethod

  Method ParsedValue(value$)
    If _callback
      _callback(SP_VALUE, value, _context)
    EndIf
  EndMethod

  Method ParsedNodeOpening()
    If _callback
      _callback(SP_NODE_OPENING, "{", _context)
    EndIf
    _openings.AddLast(New TSparseException.Init("Finalized parser with incomplete document - no matching }", ..
                                                _line, _column))
  EndMethod

  Method ParsedNodeClosing()
    If _callback
      _callback(SP_NODE_CLOSING, "}", _context)
    EndIf
    _openings.RemoveLast()
  EndMethod

  Method _ParseWhitespace(char%)
    If _mode = SP_READ_NAME
      ParsedName(_GrabAndResetBuffer())
      _mode = SP_FIND_VALUE
    ElseIf Not ((_consume_whitespace Or _last_char = char) ..
                Or _mode = SP_FIND_NAME Or _mode = SP_FIND_VALUE)
      _BufferChar(char, False)
    EndIf
  EndMethod

  Method _ParseOpening()
    Local lastMode% = _mode
    _ParseTerminateField(False, SP_FIND_NAME)
    If lastMode = SP_FIND_NAME Or lastMode = SP_READ_VALUE
      If _depth = 0 And _nameless_roots
        _depth :+ 1
        ParsedName("")
        ParsedNodeOpening()
      Else
        Throw New TSparseException.Init("Invalid character { - expected name.", ..
                                        _line, _column)
      EndIf
    Else
      _depth :+ 1
      ParsedNodeOpening()
    EndIf
  EndMethod

  Method _ParseClosing()
    _ParseTerminateField(True, SP_FIND_NAME)
    If _depth = 0
      Throw New TSparseException.Init("Unexpected } - no matching {.", ..
                                      _line, _column)
    EndIf
    _depth :- 1
    ParsedNodeClosing()
  EndMethod

  Method _ParseTerminateField(termValue%, nextMode%)
    Select _mode
    Case SP_READ_NAME, SP_FIND_VALUE
      If _mode = SP_READ_NAME Then
        ParsedName(_GrabAndResetBuffer())
      EndIf
      If termValue Then
        ParsedValue("")
      EndIf
    Case SP_READ_VALUE
      ParsedValue(_GrabAndResetBuffer())
    EndSelect
    _mode = nextMode
  EndMethod

  Method _GrabAndResetBuffer$()
    If _trim_spaces And _space_count > 0
      _buffer_off :- _space_count
    EndIf
    Local ret$ = String.FromShorts(_buffer, _buffer_off)
    _buffer_off = 0
    _space_count = 0
    Return ret
  EndMethod

  Method _BufferChar(char:Short, escaped%)
    _buffer = ExpandBufferToFit(_buffer, _buffer_off + 1)
    If char <> SP_SPACE And char <> SP_TAB Or escaped
      _space_count = 0
    ElseIf char = SP_SPACE or char = SP_TAB
      _space_count :+ 1
    EndIf
    _buffer[_buffer_off] = char
    _buffer_off :+ 1
  EndMethod
EndType
