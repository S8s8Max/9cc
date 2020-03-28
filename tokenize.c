#include "9cc.h"

char *user_input;
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

Token *consume_ident(void) {
  if (token->kind != TK_IDENT)
    return NULL;
  Token *t = token;
  token = token->next;
  return t;
}

//次のトークンが期待している記号のとき、トークンを1つ読み
//それ以外の場合にはエラーを報告する。
void expect(char *op) {
  if (token->kind != TK_RESERVED || strlen(op) != token->len || memcmp(token->str, op, token->len))
    error_at(token->str, "expected \"%s\"", op);
  token = token->next;
}

//次のトークンが数値の場合、トークンを1つ読んでその値を返す
//それ以外の場合にはエラーを報告。
long expect_number(void) {
  if (token->kind != TK_NUM)
    error(token->str, "数ではありません");
  long val = token->val;
  token = token->next;
  return val;
}

bool at_eof(void) {
  return token->kind == TK_EOF;
}

//新しいトークンを作成してcurに繋げる。
static Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  tok->len = len;
  cur->next = tok;
  return tok;
}

static bool startswith(char *p, char *q) {
  return strncmp(p, q, strlen(q)) == 0;
}

static bool is_alpha(char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

static bool is_alnum(char c) {
  return is_alpha(c) || ('0' <= c && c <= '9');
}

//入力文字列ｐをトークナイズしてそれを返す
Token *tokenize(void) {
  char *p = user_input;
  Token head = {};
  head.next = NULL;
  Token *cur = &head;

  while (*p) {
    //空文字をスキップ
    if (isspace(*p)) {
      p++;
      continue;
    }
    
    //Keywords
    if (startswith(p, "return") && !is_alnum(p[6])) {
      cur = new_token(TK_RESERVED, cur, p, 6);
      p += 6;
      continue;
    }

    //Identifier
    if ('a' <= *p && *p <= 'z') {
      cur = new_token(TK_RESERVED, cur, p++, 1);
      continue;
    }
    
    //Multi-letter punctuator
    if (startswith(p, "==") || startswith(p, "!=") || startswith(p, "<=") || startswith(p, ">=")) {
      cur = new_token(TK_RESERVED, cur, p, 2);
      p += 2;
      continue;
    }
    
    //Single-letter punctuator
    if (ispunct(*p)) {
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
    
    if ('a' <= *p && *p <= 'z') {
        cur = new_token(TK_IDENT, cur, p++,0);
        cur->len = 1;
    }

    error_at(p, "トークナイズできません");
  }
  
  new_token(TK_EOF, cur, p, 0);
  return head.next;
}