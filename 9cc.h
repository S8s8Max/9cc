#define _GNU_SOURCE
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Type Type;
//
// tokenize.c
//

// Token
typedef enum {
  TK_RESERVED, //記号
  TK_IDENT, //識別子
  TK_NUM, //整数トークン
  TK_EOF, //入力の終わりを表す
} TokenKind;

//Kinds of Token
typedef struct Token Token;
struct Token {
  TokenKind kind; //トークンの型
  Token *next;    //次の入力のトークン
  int val;        //kindがTK_NUMの場合の値
  char *str;      //トークン文字列
  int len;        //トークンの長さ
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

extern char *user_input;
extern Token *token;


//
// parse.c
//

typedef struct Var Var;
struct Var {
  char *name; // variable name
  Type *ty;
  int offset; // Offset from RBP
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
  ND_ADDR,//unary &
  ND_DEREF,//unary *
  ND_RETURN, // "return"
  ND_IF,  //"if"
  ND_WHILE,//"while"
  ND_FOR, //"for"
  ND_BLOCK,//{・・・}
  ND_FUNCALL,//Function call
  ND_EXPR_STMT,//Epression statement
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
  // Block
  Node *body;
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

Function *program(void);

//
// typing.c
//

typedef enum { TY_INT, TY_PTR } TypeKind;

struct Type {
  TypeKind kind;
  Type *base;
};

extern Type *int_type;

bool is_integer(Type *ty);
Type *pointer_to(Type *base);
void add_type(Node *node);

//
// codegen.c
//

void codegen(Function *prog);