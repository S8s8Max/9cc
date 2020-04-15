#define _GNU_SOURCE
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Type Type;
typedef struct Member Member;
//
// tokenize.c
//

// Token
typedef enum {
  TK_RESERVED, //keywords or punctuators
  TK_IDENT, //identifiers
  TK_STR,  //string literal
  TK_NUM, //integer literals
  TK_EOF, //end-of-file markers
} TokenKind;

//Kinds of Token
typedef struct Token Token;
struct Token {
  TokenKind kind; //Token kind
  Token *next;    //Next token
  int val;        //if kind is TK_NUM, its value
  char *str;      //Token string
  int len;        //Token length

  char *contents; //string literal contents including terminating '\0'
  char cont_len;  //string literal length
};

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
void error_tok(Token *tok, char *fmt, ...);
Token *peek(char *s);
Token *consume(char *op);
Token *consume_ident(void);
void expect(char *op);
long expect_number(void);
char *expect_ident(void);
bool at_eof(void);
Token *tokenize(void);

extern char *filename;
extern char *user_input;
extern Token *token;


//
// parse.c
//

typedef struct Var Var;
struct Var {
  char *name; // variable name
  Type *ty; // Type
  bool is_local; // local or global

  int offset; // offset from RBP

  char *contents;
  int cont_len;
};

typedef struct VarList VarList;
struct VarList {
  VarList *next;
  Var *var;
};

//AST node
typedef enum {
  ND_ADD, //num + num
  ND_PTR_ADD,// ptr + num or num + ptr
  ND_SUB, // num - num
  ND_PTR_SUB,// ptr - num
  ND_PTR_DIFF,//ptr - ptr
  ND_MUL, // *
  ND_DIV, // /
  ND_EQ,  // ==
  ND_NE,  // !=
  ND_LT,  // <
  ND_LE,  // <=
  ND_ASSIGN,// =
  ND_MEMBER,//. (struct member access)
  ND_ADDR,//unary &
  ND_DEREF,//unary *
  ND_RETURN, // "return"
  ND_IF,  //"if"
  ND_WHILE,//"while"
  ND_FOR, //"for"
  ND_BLOCK,//{・・・}
  ND_FUNCALL,//Function call
  ND_EXPR_STMT,//Epression statement
  ND_STMT_EXPR,//statement expression
  ND_VAR,// variable
  ND_NUM, // integer
  ND_NULL,//Empty statement
} NodeKind;


typedef struct Node Node;
struct Node {
  NodeKind kind; //Node kind
  Node *next;    //Next node
  Type *ty;      //Type, e.g. int or pointer to int
  Token *tok;    //Representative token

  Node *lhs;     //Left side
  Node *rhs;     //Right side
  
  // "if" or "while" statement
  Node *cond;
  Node *then;
  Node *els;
  Node *init;
  Node *inc;
  // Block or statement expressions
  Node *body;
  //struct member access
  Member *member;
  //Function call
  char *funcname;
  Node *args;

  Var *var;    //Used if kind == ND_VAR
  long val;       //Used if kind == ND_NUM
};

typedef struct Function Function;
struct Function {
  Function *next;
  char *name;
  VarList *params;
  Node *node;
  VarList *locals;
  int stack_size;
};

typedef struct {
  VarList *globals;
  Function *fns;
} Program;

Program *program(void);

//
// typing.c
//

typedef enum {
  TY_CHAR,
  TY_SHORT,
  TY_INT,
  TY_LONG,
  TY_PTR,
  TY_ARRAY,
  TY_STRUCT,
} TypeKind;

struct Type {
  TypeKind kind;
  int size;
  int align;
  Type *base;
  int array_len;
  Member *members;
};

//Struct members
struct Member {
  Member *next;
  Type *ty;
  char *name;
  int offset;
};

extern Type *char_type;
extern Type *short_type;
extern Type *int_type;
extern Type *long_type;

bool is_integer(Type *ty);
int align_to(int n, int align);
Type *pointer_to(Type *base);
Type *array_of(Type *base, int size);
void add_type(Node *node);

//
// codegen.c
//

void codegen(Program *prog);