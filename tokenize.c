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
static void verror_at(char *loc, char *fmt, va_list ap) {
  int pos = loc - user_input;
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s", pos, "");//pos個の空白を出力
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

void error_at(char *loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  verror_at(loc, fmt, ap);
}

void error_tok(Token *tok, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  verror_at(tok->str, fmt, ap);
}

//次のトークンが期待している記号のとき、トークンを1つ読み
//真を返す。それ以外の場合には偽を返す。
Token *consume(char *op) {
  if (token->kind != TK_RESERVED ||  strlen(op) != token->len || memcmp(token->str, op, token->len))
    return NULL;
  Token *t = token;
  token = token->next;
  return t;
}

Token *peek(char *s) {
  if (token->kind != TK_RESERVED || strlen(s) != token->len || strncmp(token->str, s, token->len))
    return NULL;
  return token;
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
void expect(char *s) {
  if (!peek(s));
    error_tok(token, "expected \"%s\"", s);
  token = token->next;
}

//次のトークンが数値の場合、トークンを1つ読んでその値を返す
//それ以外の場合にはエラーを報告。
long expect_number(void) {
  if (token->kind != TK_NUM)
    error_tok(token, "expected a number");
  long val = token->val;
  token = token->next;
  return val;
}

char *expect_ident(void) {
  if (token->kind != TK_RESERVED)
    error_tok(token, "expected an identifier");
  char *s = strndup(token->str, token->len);
  token = token->next;
  return s;
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

static char *starts_with_reserved(char *p) {
  //keywords
  static char *kw[] = {"return", "if", "else", "while", "for", "int", "char", "sizeof"};

  for (int i=0; i < sizeof(kw) / sizeof(*kw); i++) {
    int len = strlen(kw[i]);
    if (startswith(p, kw[i]) && !is_alnum(p[len]))
      return kw[i];
  }

  static char *ops[] = {"==", "!=", "<=", ">="};

  for (int i=0; i < sizeof(ops) / sizeof(*ops); i++)
    if (startswith(p, ops[i]))
      return ops[i];
    
  return NULL;
}

static char get_escape_char(char c) {
  switch (c) {
  case 'a': return '\a';
  case 'b': return '\b';
  case 't': return '\t';
  case 'n': return '\n';
  case 'v': return '\v';
  case 'f': return '\f';
  case 'r': return '\r';
  case 'e': return 27;
  case '0': return 0;
  default: return c;
  }
}

static Token *read_string_literal(Token *cur, char *start) {
  char *p = start + 1;
  char buf[1024];
  int len = 0;

  for (;;) {
    if (len == sizeof(buf))
      error_at(start, "string literal too large");
    if (*p == '\0')
      error_at(start, "unclosed string literal");
    if (*p == '"')
      break;

    if (*p == '\\') {
      p++;
      buf[len++] = get_escape_char(*p++);
    } else {
      buf[len++] = *p++;
    }
  }

  Token *tok = new_token(TK_STR, cur, start, p - start + 1);
  tok->contents = malloc(len + 1);
  memcpy(tok->contents, buf, len);
  tok->contents[len] = '\0';
  tok->cont_len = len + 1;
  return tok;
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

    //String literal
    if (*p == '"') {
      cur = read_string_literal(cur, p);
      p += cur->len;
      continue;
    }
    
    //Keywords
    char *kw = starts_with_reserved(p);
    if (kw) {
      int len = strlen(kw);
      cur = new_token(TK_RESERVED, cur, p, len);
      p += len;
      continue;
    }

    //Identifier
    if (is_alpha(*p)) {
      char *q = p++;
      while (is_alnum(*p))
        p++;
      cur = new_token(TK_RESERVED, cur, q, p - q);
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

    error_at(p, "invalid token");
  }
  
  new_token(TK_EOF, cur, p, 0);
  return head.next;
}