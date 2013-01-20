SuperStrict

Import cower.sparse

Function Handler(msg%, str$, ctx:Object)
  Print msg + " -> " + str
End Function

Local options% = SP_TRIM_TRAILING_SPACES | SP_NAMELESS_ROOT_NODES
Local source$ = LoadString("../ruby/test.sp")
Local parser:TSparseParser = New TSparseParser

parser.Init Null, Null, options
parser.Parse source
parser.Finalize
