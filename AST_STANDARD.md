# RPGLang's AST Standard

### Node Types
**Non-Terminal:**
1. Operand (`OP_TYPE`) - stores `OpType`
2. Control Flow (`CTRL_TYPE`) - stores `CtrlType`
**Terminal:**
1. Identifier (`IDENT_TYPE`) - stores `StringView` (sized string)
2. Number (`NUM_TYPE`) - stores `double`
3. Variable Type (`VAR_TYPE_TYPE`) - stores `VarType`

![Operand Orphan](assets/ast_standard/op_orphan.svg)
![Control Flow Orphan](assets/ast_standard/ctrl_orphan.svg)

![Identifier Orphan](assets/ast_standard/ident_orphan.svg)
![Number Orphan](assets/ast_standard/num_orphan.svg)
![Variable Type Orphan](assets/ast_standard/type_orphan.svg)

_Figures 1-5. Simplified orphan nodes of all types_

### AST Node Connection Rules
