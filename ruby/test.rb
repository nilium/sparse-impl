#!/usr/bin/env ruby -I. -w

require 'sparse'
require 'pp'

FLOAT_TYPE_REGEX = /^\d*\.\d+([eE]-?\d+)?|\d+([eE]-?\d+)$/
INT_TYPE_REGEX = /^\d$/
HEX_TYPE_REGEX = /^0[xX]([a-fA-F0-9]+)$/

class Handler

  attr_reader :root

  def initialize
    @root = {}
    @name = ''
    @last_root = 0
  end

  def parsedName name
    @name = case name
      when FLOAT_TYPE_REGEX then name.to_f
      when INT_TYPE_REGEX then name.to_i
      when HEX_TYPE_REGEX then $1.to_i(16)
      else
        if name.empty? && @root[:owner].nil?
          @last_root += 1 while @root.include? @last_root
          name = @last_root
        end
        name
      end
  end

  def parsedValue value
    value = case value.downcase
      when "", "true", "yes" then true
      when "false", "no" then false
      when FLOAT_TYPE_REGEX then value.to_f
      when INT_TYPE_REGEX then value.to_i
      when HEX_TYPE_REGEX then $1.to_i(16)
      else value
      end
    @root[@name] = value
  end

  def parsedNodeOpening
    @root = @root[@name] = {:owner => @root}
  end

  def parsedNodeClosing
    last_root = @root
    @root = last_root[:owner]
    last_root.delete(:owner)
  end
end

source = File.read('test.sp')
handler = Handler.new
parser = Sparse::Parser.new(handler, [:consume_whitespace, :trim_trailing_spaces, :nameless_root_nodes])
parser.parse(source)
parser.finalize
pp handler.root
