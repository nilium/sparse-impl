SuperStrict

Import cower.sparse

Function Handler(msg%, str$, ctx:Object)
  Print msg + " -> " + str
End Function

Local options% = SP_TRIM_TRAILING_SPACES | SP_NAMELESS_ROOT_NODES
Local parser:TSparseParser = New TSparseParser.Init(Handler, Null, options)
Local source$ = LoadString("../../../ruby/test.sp")

parser.Parse source
parser.Finalize
