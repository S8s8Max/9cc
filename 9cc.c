#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//トークンの種類
typedef enum {
  TK_RESERVED, //記号
  TK_NUM, //整数トークン
  TK_EOF, //入力の終わりを表す
} TokenKind;

//トークンの型
typedef struct Token Token;
struct Token {
  TokenKind kind; //トークンの型
  Token *next;    //次の入力のトークン
  int val;        //kindがTK_NUMの場合の値
  char *str;      //トークン文字列
  int len;        //トークンの長さ
};

//入力プログラム
char *user_input;

//現在着目しているトークン
Token *token;

//エラーを報告する
void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

//エラーを報告するための関数
//printfと同じ引数をとる
void error_at(char *loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  int pos = loc - user_input;
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s", pos, "");//pos個の空白を出力
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

//次のトークンが期待している記号のとき、トークンを1つ読み
//真を返す。それ以外の場合には偽を返す。
bool consume(char *op) {
  if (token->kind != TK_RESERVED ||  strlen(op) != token->len || memcmp(token->str, op, token->len))
    return false;
  token = token->next;
  return true;
}

//次のトークンが期待している記号のとき、トークンを1つ読み
//それ以外の場合にはエラーを報告する。
void expect(char op) {
  if (token->kind != TK_RESERVED || strlen(op) != token->len || memcmp(token->str, op, token->len))
    error_at(token->str, "expected \"%s\"", op);
  token = token->next;
}

//次のトークンが数値の場合、トークンを1つ読んでその値を返す
//それ以外の場合にはエラーを報告。
int expect_number() {
  if (token->kind != TK_NUM)
    error("数ではありません");
  int val = token->val;
  token = token->next;
  return val;
}

bool at_eof() {
  return token->kind == TK_EOF;
}

//新しいトークンを作成してcurに繋げる。
Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  tok->len = len;
  cur->next = tok;
  return tok;
}

bool startwith(char *p, char *q) {
  return memcmp(p, q, strlen(q)) == 0;
}

//入力文字列ｐをトークナイズしてそれを返す
Token *tokenize(char *p) {
  char *p = user_input;
  Token head;
  head.next = NULL;
  Token *cur = &head;

  while (*p) {
    //空文字をスキップ
    if (isspace(*p)) {
      p++;
      continue;
    }
    
    //Multi-letter punctuator
    if (startwith(p, "==") || startwith(p, "!=") || startwith(p, "<=") || startwith(p, ">=")) {
      cur = new_token(TK_RESERVED, cur, p, 2);
      p += 2;
      continue;
    }
    
    //Single-letter punctuator
    if (strchr("+-*/()<>", *p)) {
      cur = new_token(TK_RESERVED, cur, p++, 1);
      continue;
    }

    //Integer literal
    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p, 0);
      char *q = p;
      cur->val = strtol(p, &p, 10);
      cur->len = p - q;
      continue;
    }
    
    error_at(p, "トークナイズできません");
  }
  
  new_token(TK_EOF, cur, p, 0);
  return head.next;
}

//抽象構文木のノードの種類
typedef enum {
  ND_ADD, // +
  ND_SUB, // -
  ND_MUL, // *
  ND_DIV, // /
  ND_EQ,  // ==
  ND_NE,  // !=
  ND_LT,  // <
  ND_LE,  // <=
  ND_NUM, // integer
} NodeKind;

//抽象構文木のノードの型
typedef struct Node Node;
struct Node {
  NodeKind kind; //ノードの型
  Node *lhs;     //左辺
  Node *rhs;     //右辺
  int val;       //kindがND_NUMの場合のみ使う
};


Node *new_node(NodeKind kind) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  return node;
}

Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = new_node(kind);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_num(int val) {
  Node *node = new_node(ND_NUM);
  node->val = val;
  return node;
}

Node *expr();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();

//パーサ
//expr = equality
Node *expr() {
  return equality();
}
//equality = relational("==" relational | "!=" relational)*
Node *equality() {
  Node *node = relational();

  for (;;) {
    if (consume('=='))
      node = new_binary(ND_EQ, node, relational());
    else if (consume('!='))
      node = new_binary(ND_NE, node, relational());
    else
      return node;
  }
}

//relational = add ("<" add | "<=" add | ">" add | ">=" add)*
Node *relational() {
  Node *node = add();

  for (;;) {
    if (consume('<'))
      node = new_binary(ND_LT, node, add());
    else if (consume('<='))
      node = new_binary(ND_LE, node, add());
    else if (consume('>'))
      node = new_binary(ND_LT, add(), node);
    else if (consume(">="))
      node = new_binary(ND_LE, add(), node);
    else
      return node;
  }
}

//add = mul("+" mul | "-" mul)*
Node *add() {
  Node *node = mul();

  for (;;) {
    if (consume("+"))
      node = new_binary(ND_ADD, node, mul());
    else if (consume("-"))
      node = new_binary(ND_SUB, node, mul());
    else
      return node;
  }
}

//mul = unary ("*" unary | "/" unary)*
Node *mul() {
  Node *node = unary();

  for (;;) {
    if (consume("*"))
      node = new_binary(ND_MUL, node, unary());
    else if (consume("/"))
      node = new_binary(ND_DIV, node, unary());
    else
      return node;
  }
}

//unary = ("+" | "-")? unary
Node *unary() {
  if (consume("+"))
    return unary();
  if (consume("-"))
    return new_binary(ND_SUB, new_num(0), unary());
  return primary();
}

//primary = "(" expr ")" | num
Node *primary() {
  //次のトークンが"("なら、"(" expr ")"のはず
  if (consume("(") {
    Node *node = expr();
    expect(")");
    return node;
  }

  //そうで無ければ数値のはず
  return new_num(expect_number());
}

//
//Code generator
//

void gen(Node *node) {
  if (node->kind == ND_NUM) {
    printf("  push %d\n", node->val);
    return;
  }

  gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->kind) {
  case ND_ADD:
    printf("  add rax, rdi\n");
    break;
  case ND_SUB:
    printf("  sub rax, rdi\n");
    break;
  case ND_MUL:
    printf("  imul rax, rdi\n");
    break;
  case ND_DIV:
    printf("  cqo\n");
    printf("  idiv rdi\n");
    break;
  case ND_EQ:
    printf("  cmp rax, rdi\n");
    printf("  sete al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_NE:
    printf("  cmp rax, rdi\n");
    printf("  setne al\n");
    printf(" movzb rax, al\n");
    break;
  case ND_LT:
    printf("  cmp rax, rdi\n");
    printf(" setl al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_LE:
    printf("  cmp rax, rdi\n");
    printf("  setle al\n");
    printf("  movzb rax, al\n");
    break;
  }

  printf("  push rax\n");
}

int main(int argc, char **argv) {
  if (argc != 2)
    error("%s: 引数の個数が正しくありません", argv[0]);
  
  //2トークナイズしてパース
  user_input = argv[1];
  token = tokenize(user_input);
  Node *node = expr();

  //2アセンブリの前半部分を出力
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  //2抽象構文木を下りながらコード生成
  gen(node);

  //2スタックトップに式全体の値が残っているはずなので
  //それをRAXにロードして関数からの返値とする
  printf("  pop rax\n");
  printf("  ret\n");
  return 0;
}