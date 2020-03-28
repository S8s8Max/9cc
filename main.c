#include "9cc.h"

int main(int argc, char **argv) {
  if (argc != 2)
    error("%s: 引数の個数が正しくありません", argv[0]);
  
  //トークナイズしてパース
  user_input = argv[1];
  token = tokenize();
  Node *node = program();
  codegen(node);
  return 0;
}