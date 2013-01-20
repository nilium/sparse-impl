SP_FIND_NAME, SP_FIND_VALUE, SP_READ_NAME, SP_READ_VALUE, SP_READ_COMMENT = range(5)

class SparseHandler(object):
    def __init__(self):
        super(SparseHandler, self).__init__()

    def parsed_name(self, name):
        raise NotImplementedError("parsedName not implemented in SparseHandler")

    def parsed_value(self, value):
        raise NotImplementedError("parsedValue not implemented in SparseHandler")

    def parsed_node_opening(self):
        raise NotImplementedError("parsedNodeOpening not implemented in SparseHandler")

    def parsed_node_closing(self):
        raise NotImplementedError("parsedNodeClosing not implemented in SparseHandler")

class SparseException(Exception):
    """Sparse parsing exception"""
    def __init__(self, message, position = None):
        super(SparseException, self).__init__()
        self._message = message
        self._position = position

    def __str__(self):
        if self._position:
            return self._message
        else:
            return "[{line}:{col}] {msg}".format(line = self._position[0],
                                                 col = self._position[1],
                                                 msg = self._message)

class SparseParser(object):
    """Sparse document parser"""
    def __init__(self, handler, allow_nameless_roots = False,
                 trim_trailing_spaces = True, consume_whitespace = False):
        super(SparseParser, self).__init__()

        self._nameless_roots = allow_nameless_roots
        self._trim_spaces = trim_trailing_spaces
        self._consume_spaces = consume_whitespace

        self._last_char = ''
        self._char_buffer = []
        self._finished = False
        self._in_escape = False
        self._depth = 0
        self._line = 1
        self._column = 1
        self._mode = SP_FIND_NAME
        self._space_count = 0
        self._openings = []
        self._handler = handler

    def parsed_name(self, name):
        if self._handler:
            self._handler.parsed_name(name)

    def parsed_value(self, value):
        if self._handler:
            self._handler.parsed_value(value)

    def parsed_node_opening(self):
        self._openings.append((self._line, self._column))
        if self._handler:
            self._handler.parsed_node_opening()

    def parsed_node_closing(self):
        self._openings.pop()
        if self._handler:
            self._handler.parsed_node_closing()

    def parse(self, source):
        if self._finished:
            raise SparseException("Attempt to continue parsing using finalized parser")

        for char in source:
            if self._mode == SP_READ_COMMENT:
                if char == "\n":
                    self._mode = SP_FIND_NAME
            elif self._in_escape:
                if char == 'n':
                    char = '\n'
                elif char == 'n':
                    char = '\n'
                elif char == 'r':
                    char = '\r'
                elif char == 'a':
                    char = '\a'
                elif char == 'b':
                    char = '\b'
                elif char == 'f':
                    char = '\f'
                elif char == 't':
                    char = '\t'
                elif char == '0':
                    char = '\0'
                self._buffer(char)
            elif char == ' ' or char == '\t':
                self._parse_whitespace(char)
            elif char == '\n' or char == ';' or char == '#':
                self._parse_terminal(char)
            elif char == '{':
                self._parse_opening()
            elif char == '}':
                self._parse_closing()
            elif char == '\\':
                self._in_escape = True
            else:
                if self._mode == SP_FIND_NAME:
                    self._mode = SP_READ_NAME
                elif self._mode == SP_FIND_VALUE:
                    self._mode = SP_READ_VALUE
                self._buffer(char)

            self._last_char = char

            if char == '\n':
                self._line += 1
                self._column = 1
            else:
                self._column += 1

    def finalize(self):
        if self._finished:
            raise SparseException("Attempt to finalize already-finalized parser")

        self._parse_terminate_field(True)

        if self._depth > 0:
            last_marker = self._openings[-1]
            raise SparseException('Finalized parser with incomplete document - expected closing } to match { at [{line}, {col}]'.format(
                                  line = last_marker[0], col = last_marker[1]))

        self._finished = True

    def _parse_terminate_field(self, terminate_value):
        if self._mode == SP_READ_NAME:
            self.parsed_name(self.grab_reset_buffer())
            if terminate_value:
                self.parsed_value('')
        elif self._mode == SP_FIND_VALUE and terminate_value:
            self.parsed_value('')
        elif self._mode == SP_READ_VALUE:
            # terminate_value is necessarily true in this case since the parser
            # has already begun to read a value.
            self.parsed_value(self.grab_reset_buffer())

        # In all but one case, this is true. In the one odd case (comments),
        # this is actually a wasted instruction (see the end of _parse_terminal)
        # but it's not really worth trying to optimize something so small.
        self._mode = SP_FIND_NAME

    def _parse_whitespace(self, char):
        if self._mode == SP_READ_NAME:
            self.parsed_name(self.grab_reset_buffer())
            self._mode = SP_FIND_VALUE
        elif not ((self._consume_spaces or self._last_char == char) or
                  self._mode == SP_FIND_NAME or
                  self._mode == SP_FIND_VALUE):
            self._buffer(char)

    def _parse_terminal(self, char):
        self._parse_terminate_field(True)

        if char == "#":
            self._mode = SP_READ_COMMENT


    def _parse_opening(self):
        # store mode since _parse_terminate_field modifies it
        last_mode = self._mode
        self._parse_terminate_field(False)
        if last_mode == SP_FIND_NAME or last_mode == SP_READ_VALUE:
            if self._depth == 0 and self._nameless_roots:
                self._depth += 1
                self.parsed_name('')
                self.parsed_node_opening()
            else:
                raise SparseException('Invalid character { - expected name.',
                                      (self._line, self._column))
        else:
            self._depth += 1
            self.parsed_node_opening()

    def _parse_closing(self):
        self._parse_terminate_field(True)
        if self._depth == 0:
            raise SparseException('Unexpected } - no matching {.',
                                  (self._line, self._column))

        self._depth -= 1

        self.parsed_node_closing()

    def grab_reset_buffer(self):
        temp = None
        if self._trim_spaces and self._space_count > 0:
            temp = ''.join(self._char_buffer[:-self._space_count])
        else:
            temp = ''.join(self._char_buffer)
        self._char_buffer[:] = []
        self._space_count = 0
        return temp

    def _buffer(self, char, escape = False):
        if char != ' ' or escape:
            self._space_count = 0
        elif char == ' ' or char == '\t':
            self._space_count += 1
        self._char_buffer.append(char)
