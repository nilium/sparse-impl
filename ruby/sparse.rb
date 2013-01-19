module Sparse

  TRIM_TRAILING_SPACES = :trim_trailing_spaces
  NAMELESS_ROOT_NODES = :nameless_root_nodes
  CONSUME_WHITESPACE = :consume_whitespace
  DEFAULT_OPTIONS = [TRIM_TRAILING_SPACES]

end

class Sparse::Parser

  # modes
  # :read_comment
  # :find_name
  # :find_value
  # :read_name
  # :read_value

  attr_reader :finished

  def initialize(handler, options)
    options = DEFAULT_OPTIONS if options.empty?
    @nameless_roots = options.include?(:nameless_root_nodes)
    @consume_whitespace = options.include?(:consume_whitespace)
    @trim_spaces = options.include?(:trim_trailing_spaces)
    @last_char = ""
    @buffer = ""
    @finished = false
    @in_escape = false
    @depth = 0
    @line = 1
    @column = 1
    @mode = :find_name
    @trailing_space_count = 0
    @openings = []
    @handler = handler
  end

  # Note: if parsing fails, the parser's state might as well be a mess and
  # further use of a parser may result in undefined behavior
  def parse(source)
    raise "Attempt to continue parsing using finalized parser" if @finished

    source.each_char() { |char|
      if @mode == :read_comment
        if char == "\n"
          newline()
          @mode = :find_name
        end
      elsif @in_escape == true
        buffer(case char
               when 'n' then "\n"
               when 'r' then "\r"
               when 'a' then "\a"
               when 'b' then "\b"
               when 'f' then "\f"
               when 't' then "\t"
               else char
               end, true)
        @in_escape = false
      else
        case char
        when ' ', "\t"
          if @mode == :read_name
            parsedName(grab_and_reset_buffer())
            @mode = :find_value
          elsif ! ((@consume_whitespace && @last_char == char) || @mode == :find_name || @mode == :find_value)
            buffer char
          end

        when '{'
          case @mode
          when :read_name, :find_value
            parsedName(grab_and_reset_buffer()) if @mode == :read_name
            parsedNodeOpening()
            @depth += 1

          when :read_value, :find_name
            parsedValue(grab_and_reset_buffer()) if @mode == :read_value
            if @depth == 0 && @nameless_roots
              @depth += 1
              parsedName('')
              parsedNodeOpening()
            else
              raise error_string('Invalid character { - expected name.')
            end

          else
            raise error_string('Invalid character { - expected name.')
          end

          @mode = :find_name

          when '}'
            case @mode
            when :read_name, :find_value
              parsedName(grab_and_reset_buffer()) if @mode == :read_name
              parsedValue('')

            when :read_value
              parsedValue(grab_and_reset_buffer())
            end

            if @depth == 0
              raise error_string("Unexpected } - no matching {.")
            end

            @depth -= 1
            @mode = :find_name

            parsedNodeClosing()

          when "\n", '#', ';'
            newline() if char == "\n"
            case @mode
            when :read_name, :find_value
              parsedName(grab_and_reset_buffer()) if @mode == :read_name
              parsedValue('')
            when :read_value
              parsedValue(grab_and_reset_buffer())
            end

            @mode = char == '#' ? :read_comment : :find_name

          when "\\"
            @in_escape = true

          else
            if @mode == :find_name
              @mode = :read_name
            elsif @mode == :find_value
              @mode = :read_value
            end

            buffer(char)

        end
      end

      @last_char = char
      @column += 1
    }
  end

  def finalize()
    raise "Attempt to finalize already-finalized parser" if @finished

    case @mode
    when :read_name, :find_value
      parsedName(grab_and_reset_buffer()) if mode == :read_name
      parsedValue('')
    when :read_value
      parsedValue(grab_and_reset_buffer())
    end

    if @depth > 0
      raise error_string("Finalized parser with incomplete document - expected closing } to match { at #{self.class.line_column_string(*(@openings.last))}")
    end

    @buffer = nil

    @finished = true
  end

  def parsedName(name)
    raise "Attempt to continue parsing using finalized parser" if @finished
    @handler.parsedName(name) if @handler.respond_to?(:parsedName)
  end

  def parsedValue(value)
    raise "Attempt to continue parsing using finalized parser" if @finished
    @handler.parsedValue(value) if @handler.respond_to?(:parsedValue)
  end

  def parsedNodeOpening()
    raise "Attempt to continue parsing using finalized parser" if @finished
    @handler.parsedNodeOpening if @handler.respond_to?(:parsedNodeOpening)
    @openings << [@line, @column]
  end

  def parsedNodeClosing()
    raise "Attempt to continue parsing using finalized parser" if @finished
    @handler.parsedNodeClosing if @handler.respond_to?(:parsedNodeClosing)
    @openings.pop
  end

  protected

  def newline()
    @line += 1
    @column = 0
  end

  def error_string(message)
    "#{self.class.line_column_string @line, @column} #{message}"
  end

  def buffer(char, escape = false)
    if char != ' ' || escape
      @trailing_space_count = 0
    elsif char == ' ' || char == "\t"
      @trailing_space_count += 1
    end

    @buffer << char
  end

  def grab_and_reset_buffer()
    buf = if @trim_spaces && @trailing_space_count > 0
      @buffer[0 ... @buffer.length - @trailing_space_count]
    else
      @buffer
    end
    @buffer = ''
    @trailing_space_count = 0
    buf
  end

  def self.line_column_string(line, column)
    "[#{line}:#{column}]"
  end

end
