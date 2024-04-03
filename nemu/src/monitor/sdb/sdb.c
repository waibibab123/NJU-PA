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
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"
#include <memory/paddr.h>

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  nemu_state.state = NEMU_QUIT;
  return -1;
}

static int cmd_help(char *args);
static int cmd_si(char *args);
static int cmd_info(char *args);
static int cmd_x(char *args);
static int cmd_p(char *args);
static int cmd_test(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si","si [n]: run n steps,n is optional,if not given,run 1 step", cmd_si},
  { "info","info [char]: if char is r,print regs,else print watchpoints", cmd_info},
  { "x","x [n] [addr]:print n * 4 Bytes datas from addr(hex) in pmem",cmd_x}, 
  { "p","p [exp]:calculate the value of [exp] and print it",cmd_p},
  { "test","it read from file \"input\" to test the accuracy of calculation function of nemu",cmd_test},

  /* TODO: Add more commands */

};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

static int cmd_si(char *args){
  int step;
  if(args == NULL)
  step = 1;
  else 
  sscanf(args,"%d",&step);
  cpu_exec(step);
  return 0;
}

static int cmd_info(char *args){
  if(args == NULL)
    printf("please print args\n");
  else if(strcmp(args, "r") == 0)
    isa_reg_display();
// else if(strcmp(args, "w") == 0)
//   sdb_watchpoint_display();
  return 0;
}

static int cmd_x(char *args){
  char *n = strtok(args," ");
  char *baseaddr = strtok(NULL," ");
  int len = 0;
  paddr_t addr = 0;
  sscanf(n, "%d" ,&len);
  sscanf(baseaddr,"%x", &addr);
  for(int i = 0; i < len ; i ++)
  {
    printf("%x\n",paddr_read(addr,4));
    addr = addr + 4;
  }
  return 0;
 }

 static int cmd_p(char *args){
  if(args == NULL)
  {
    printf("No args\n");
    return 0;
  }
  bool flag = false;
  uint32_t res = expr(args, &flag);
  printf("the value is %u\n",res);
  return 0; 
}

static int cmd_test(char *args){
  int right_ans = 0;
  FILE *input_file = fopen("/home/dmz/ics2023/nemu/tools/gen-expr/input", "r");
    if (input_file == NULL) {
        perror("Error opening input file");
        return 1;
    }

    char record[1024];
    unsigned real_val;
    char buf[1024];

    // 循环读取每一条记录
    for (int i = 0; i < 100; i++) {
        // 读取一行记录
        if (fgets(record, sizeof(record), input_file) == NULL) {
            perror("Error reading input file");
            break;
        }

        // 分割记录，获取数字和表达式
        char *token = strtok(record, " ");
        if (token == NULL) {
            printf("Invalid record format\n");
            continue;
        }
        real_val = atoi(token); // 将数字部分转换为整数

        // 处理表达式部分，可能跨越多行
        strcpy(buf, ""); // 清空buf
        while ((token = strtok(NULL, "\n")) != NULL) {
            strcat(buf, token);
            strcat(buf, " "); // 拼接换行后的部分，注意添加空格以分隔多行内容
        }

        // 输出结果
        printf("Real Value: %u, Expression: %s\n", real_val, buf);
        bool flag = false;
        unsigned res = expr(buf,&flag);
        if(res == real_val)right_ans ++;

    }
    printf("test 100 expressions,the accuracy is %d/100\n",right_ans);
    fclose(input_file);
    return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
