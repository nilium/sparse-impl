from sparse import *
from pprint import pprint

class ParseHandler(SparseHandler):
    def __init__(self):
        super(ParseHandler, self).__init__()
        self._root = {}
        self._name = ''
        self._last_root = 0

    def root(self):
        return self._root

    def parsed_name(self, name):
        if name == '' and not '__owner' in self._root:
            self._last_root += 1
            while str(self._last_root) in self._root:
                self._last_root += 1
            name = str(self._last_root)

        self._name = name

    def parsed_value(self, value):
        if value == '':
            value = True
        self._root[self._name] = value

    def parsed_node_opening(self):
        new_root = {'__owner': self._root}
        self._root[self._name] = new_root
        self._root = new_root

    def parsed_node_closing(self):
        new_root = self._root['__owner']
        del self._root['__owner']
        self._root = new_root

source_string = None
with open('../ruby/test.sp', 'r') as test_file:
    source_string = test_file.read()

handler = ParseHandler()

parser = SparseParser(handler,
                      trim_trailing_spaces = True,
                      allow_nameless_roots = True)

parser.parse(source_string)
parser.finalize()

pprint(handler.root())
