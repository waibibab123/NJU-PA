/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <math.h>
#include <stdlib.h>


/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

enum {
  TK_NOTYPE = 256, 
  NUM = 1,
  REGISTER = 2,
  HEX = 3,
  EQ = 4,
  NOTEQ = 5,
  OR = 6,
  AND = 7,
  ZUO = 8,
  YOU = 9,
  LEQ = 10,
  YINYONG = 11,
  POINT,NEG

  /* TODO: Add more token types */

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"\\-", '-'},         // sub
  {"\\*", '*'},         // mul
  {"\\/", '/'},         // div
  {"\\(", ZUO},
  {"\\)", YOU},
  /*
   * Inset the '(' and ')' on the [0-9] bottom case Bug.
   */

  {"\\<\\=", LEQ},            // TODO
  {"\\=\\=", EQ},        // equal
  {"\\!\\=", NOTEQ},

  {"\\|\\|", OR},       // Opetor
  {"\\&\\&", AND},
  {"\\!", '!'},

  //{"\\$[a-z]*", RESGISTER},
  {"\\$[a-zA-Z]*[0-9]*", REGISTER},
  {"0[xX][0-9a-fA-F]+", HEX},
  {"[0-9]*", NUM},

  
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[680] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

//static bool  check_parentheses(int p,int q){
//  if(tokens[p].type != '(' || tokens[q].type != ')')return false;
//  int i = p;
//  int j = q;
//  while(i < j){
//    if(tokens[i].type == '('){
//      if(tokens[j].type == ')')
//      {
//        i ++;
//        j --;
//      }
////      else if(tokens[j].type == '(')return false;
//      else 
//        j --;
//    }
//    else if(tokens[i].type == ')')return false;
//    else i ++;
//  }
//  return true;
//}

static int check_parentheses(int p,int q){
  //return -1:stop calculate
  //return 0 :no need to remove parentheses
  //return 1 :need to remove parentheses
  int cnt = 0;
  for(int i = p; i <= q;i ++)
  {
    if(tokens[i].type == '(')cnt ++;
    else if(tokens[i].type == ')')cnt --;
    
    if(cnt < 0)return -1;
  }
  if(cnt != 0) return -1;
  if(tokens[p].type != '(' || tokens[q].type != ')')return 0;

  int ret = check_parentheses(p+1,q-1);
  if(ret == -1)return 0;
  else if(ret == 0||ret == 1)return 1;
  return 2;//algorithm_error
}


static int max(int a,int b){
  return a > b?a : b;
}
int len = 0;//record the amounts of tokens
static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
        Token tmp_token;

        switch (rules[i].token_type) {
          case '+':
            tmp_token.type = '+';
            tokens[nr_token ++] = tmp_token;
            break;
          case '-':
            tmp_token.type = '-';
            tokens[nr_token ++] = tmp_token;
            break;
          case '*':
            tmp_token.type = '*';
            tokens[nr_token ++] = tmp_token;
            break;
          case '/':
            tmp_token.type = '/';
            tokens[nr_token ++] = tmp_token;
            break;
          case 256:  
            break;
          case '!':
            tmp_token.type = '!';
            tokens[nr_token ++] = tmp_token;
            break;
          case 9: 
            tmp_token.type = ')';
            tokens[nr_token ++] = tmp_token;
            break;
          case 8: 
            tmp_token.type = '(';
            tokens[nr_token ++] = tmp_token;
            break;
          case 1:
            tokens[nr_token].type = 1;
            strncpy(tokens[nr_token].str, &e[position - substr_len], substr_len);
            nr_token ++;
            break;
          case 2:
            tokens[nr_token].type = 2;
            strncpy(tokens[nr_token].str, &e[position - substr_len], substr_len);
            nr_token ++;
            break;
          case 3:
            tokens[nr_token].type = 3;
            strncpy(tokens[nr_token].str, &e[position - substr_len], substr_len);
            nr_token ++;
            break;
          case 4:
            tokens[nr_token].type = 4; 
            strcpy(tokens[nr_token].str, "==");
            nr_token ++;
            break;   
          case 5:
            tokens[nr_token].type = 5;
            strcpy(tokens[nr_token].str, "!=");
            nr_token ++;
            break;
          case 6:
            tokens[nr_token].type = 6;
            strcpy(tokens[nr_token].str, "||");
            nr_token ++;
            break;
          case 7:
            tokens[nr_token].type = 7;
            strcpy(tokens[nr_token].str, "&&");
            nr_token ++;
            break;
          case 10:
            tokens[nr_token].type = 10;
            strcpy(tokens[nr_token].str, "<=");
            nr_token ++;
            break;
          default:
            printf("i = %d and No rules is com.\n",i);
            break;
        }
        break;
      }
    }
    len = nr_token;
    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

int eval(int p,int q){
  printf("p = %d,q = %d",p,q);
  if(p > q){
    printf("%d %d\n",p,q);
    assert(0);
    return -1;
  }
  else if(p == q){
    return atoi(tokens[p].str);
  }
  else if(check_parentheses(p, q) == 1){
    printf(" remove_parentheses ");
    return eval(p + 1, q - 1);
  }
  else{
    if(check_parentheses(p,q) == 2)printf("algorithm error\n");
    else if(check_parentheses(p,q) == -1)assert(0);
    int op = -1;
    int flag = -1 ;
    for(int i = p; i <= q;i ++)
    {
      if(tokens[i].type == '(')
      {
        int cnt = 1;
        while(cnt != 0)
        {
          i ++;
          if(tokens[i].type == '(')cnt ++;
          else if(tokens[i].type == ')')cnt--;
        }
      }
      
      if(flag<=7 && tokens[i].type == 6){
        flag = 7;
        op = max(op,i);
      }
      if(flag<=6 && tokens[i].type == 7){
        flag = 6;
        op = max(op,i);
      }
      if(flag<=5 && tokens[i].type == 5){
        flag = 5;
        op = max(op,i);
      }
      if(flag<=4 && tokens[i].type == 4){
        flag = 4;
        op = max(op,i);
      }
      if(flag<=3 && tokens[i].type == 10){
        flag = 3;
        op = max(op,i);
      }
      if(flag<=2 && (tokens[i].type == '+' || tokens[i].type == '-')){
        flag = 2;
        op = max(op,i);
      }
      if(flag<=1 && (tokens[i].type == '*' || tokens[i].type == '/') ){
        flag =1;
        op = max(op,i);
      }
    }
    int op_type = tokens[op].type;
    printf("%c",op_type);
    int val1 = eval(p, op - 1);
    int val2 = eval(op + 1, q);
    switch (op_type){
      case '+':
        return val1 + val2;
      case '-':
        return val1 - val2;
      case '*':
        return val1 * val2;
      case '/':
        if(val2 == 0){
          printf("  val1 = %d p = %d q = %d\n",val1,p,q);
          assert(0);
          return 0;
        }
        return val1 / val2;
      case 4:
        return val1 == val2;
      case 5:
        return val1 != val2;
      case 6:
        return val1 || val2;
      case 7:
        return val1 && val2;
      default:
        printf("No Op type");
        assert(0);
    }
  }
}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  
  //get length
  int tokens_len = 0;
  for(int i = 0;i < 670;i ++)
  {
    if(tokens[i].type == 0)
      break;
    tokens_len ++;
  }
  //init the tokens regex
  for(int i = 0;i < tokens_len; i ++)
  {
    if(tokens[i].type == 2)
    {
      bool flag = true;
      int tmp = isa_reg_str2val(tokens[i].str, &flag);
      if(flag){
        sprintf(tokens[i].str,"%d",tmp);
      }else{
      printf("Transform error\n");
      assert(0);
      }
    }
  }
  //init HEX
  for(int i = 0;i < tokens_len; i++)
  {
    if(tokens[i].type == 3)
    {
      int value = strtol(tokens[i].str, NULL,16);
      sprintf(tokens[i].str,"%d",value); 
    }
  }
  //init '-'
  for(int i = 0;i < tokens_len; i ++)
  {
    if((tokens[i].type == '-' && i > 0 &&tokens[i-1].type != NUM && tokens[i+1].type == NUM &&tokens[i-1].type != ')')
    ||
    (tokens[i].type == '-' && i == 0)) 
    
  {
    tokens[i].type = TK_NOTYPE;
    for(int j = 31;j >= 0;j --)
    {
      tokens[i+1].str[j] = tokens[i+1].str[j-1];
    }
    tokens[i+1].str[0] = '-';
    for(int j = 0;j < tokens_len; j ++){ 
    if(tokens[j].type == TK_NOTYPE)
    {
      for(int k = j+1; k < tokens_len;k ++){
        tokens[k - 1] = tokens[k];
      }
      tokens_len --;
    }
  }
  }
  }
  //init '!'
  for(int i = 0;i < tokens_len; i ++)
  {
    if(tokens[i].type == '!')
    {
      tokens[i].type = TK_NOTYPE;
      int tmp = atoi(tokens[i+1].str);
      if(tmp == 0){
        memset(tokens[i+1].str, 0 ,sizeof tokens[i+1].str);
        tokens[i+1].str[0] = '1';
      }
      else {
        memset(tokens[i+1].str, 0 ,sizeof tokens[i+1].str);
      }
      for(int j = 0; j < tokens_len ;j ++){
      if(tokens[j].type == TK_NOTYPE)
      {
        for(int k = j + 1;k < tokens_len ; k ++){
          tokens[k - 1] = tokens[k];
        }
        tokens_len --;
      }
      }
    }
  }
  //init jie yin yong 
for(int i = 0 ; i < tokens_len ; i ++)
{
  if((tokens[i].type == '*' && i > 0 && tokens[i-1].type !=NUM && tokens[i-1].type != HEX &&tokens[i-1].type != REGISTER
  && tokens[i+1].type == NUM
  ) ||(tokens[i].type == '*' && i > 0 && tokens[i-1].type != NUM && tokens[i-1].type != HEX && tokens[i-1].type != REGISTER
  && tokens[i+1].type == HEX) || (tokens[i].type == '*' && i == 0))
  {
    tokens[i].type = TK_NOTYPE;
    int temp;
    if(tokens[i+1].type == NUM)
    {
      sscanf(tokens[i+1].str,"%d",&temp);
    }
    else 
    {
      sscanf(tokens[i+1].str,"%x",&temp);
    }
    uintptr_t a = (uintptr_t)temp;
    int value = *((int*)a);
    sprintf(tokens[i+1].str,"%d",value);
    for(int j = 0;j < tokens_len; j ++)
    {
      if(tokens[j].type == TK_NOTYPE)
      {
        for(int k = j + 1; k < tokens_len ;k ++)
        {
          tokens[k - 1] = tokens[k];
        }
      }
    }
    }
 }

  //true cal
  printf("\n%d",tokens_len - 1);
  uint32_t res = 0;
  res = eval(0,tokens_len - 1);
 
  memset(tokens, 0 ,sizeof tokens);
  *success=true;  
  return res; 
}
