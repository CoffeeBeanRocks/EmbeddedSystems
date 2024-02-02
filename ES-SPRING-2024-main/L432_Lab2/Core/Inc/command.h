#include <stdint.h>
#include <queue.h>

#define COMPLETE 0
#define COLLECT_CHARS 1
#define MAX_COMMAND_LEN QUEUE_SIZE

typedef struct command {
  char * cmd_string;
  void (*cmd_function)(char * arg);
} command_t;

int parse_command (uint8_t *line, uint8_t **command, uint8_t **args);
int execute_command(uint8_t *line);
int get_command(uint8_t *command_buf);
void prompt(void);
