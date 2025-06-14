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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

// this should be enough
static char buf[660] = {};
static char code_buf[660 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

int index_buf = 0;
int choose(int n){
  int  flag = rand() % 3;
 // printf("index = %d,flag = %d.\n",index_buf,flag);
  return flag;
}

void gen_num(){
  if(index_buf > 655)return;
  int num = rand() % 100 + 1;
  int num_size = 0,num_tmp= num;
  while(num_tmp){
  num_tmp /= 10;
  num_size ++;
  }
  int x = 1;
  while(num_size)
  {
  x *= 10;
  num_size -- ;
  }
  x /= 10;
  while(num)
  {
  char c = num / x + '0';
  num %= x;
  x /= 10;
  buf[index_buf ++] = c;
  }
}

void gen(char c){
  if(index_buf > 655)return;
  buf[index_buf ++] = c;
}

void gen_rand_op(){
  if(index_buf > 655)return;
  char op[4] = {'+', '-' ,'*','/'};
  int op_position = rand() % 4;
  buf[index_buf ++] = op[op_position];
}

static void gen_rand_expr(){
//  buf[0] = '\0' ;
  if(index_buf > 655){
//    printf("overSize\n");
    index_buf = 0;}
  
  switch(choose(3)){
    case 0: 
      gen_num();

      break;
    case 1:
      gen('(');
      gen_rand_expr();
      gen(')');
      break;
    default:
      gen_rand_expr();
      gen_rand_op();
      gen_rand_expr();
      break;
    }
}



int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    index_buf = 0;
    gen_rand_expr();
    buf[index_buf] = '\0';
    long size = 0;
    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);
    
    FILE *fp_err = fopen("/tmp/.err_message","w");
    assert(fp_err != NULL);

    int ret = system("gcc /tmp/.code.c -o /tmp/.expr 2>/tmp/.err_message");
    
    fseek(fp_err, 0, SEEK_END);

    // 获取文件大小
    size = ftell(fp_err);
    fclose(fp_err);
    if (ret != 0 ||size != 0){index_buf = 0; i--; continue;}
    
    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    uint32_t result;
    ret = fscanf(fp, "%u", &result);
    pclose(fp);

    printf("%u %s\n", result, buf);
    index_buf = 0;
  }
  return 0;
}
