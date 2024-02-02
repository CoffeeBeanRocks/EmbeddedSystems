#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <main.h>
#include "interrupt.h"
#include "queue.h"
#include "command.h"

extern queue_t queue;

void help_command(char *arguments);
void lof_command(char *arguments);
void lon_command(char *arguments);
void test_command(char *arguments);

command_t commands[] = {
  {"help",help_command},
  {"lof",lof_command},
  {"lon",lon_command},
  {"test",test_command},
  {0,0}
};

int parse_command (uint8_t *line, uint8_t **command, uint8_t **args) {
	// looks for the first comma, places a NULL and captures the remainder as the arguments
	uint8_t *p;
	if((!line) || (!command) || (!args)) {
		return (-1); // Passed a bad pointer
	}

	*command = line;
	p = line;
	while (*p!=','){
		if (!*p) {
			*args = '\0';
			return(0);
		}
		p++;
	}

	*p++ = '\0'; // Replace first comma with a null
	*args = p; // The arguments are right after the comma
	return (0);
}

int execute_command(uint8_t *line)
{
	uint8_t *cmd;
	uint8_t *arg;
	command_t *p = commands;
	int success = 0;
	if (!line) {
		return (-1); // Passed a bad pointer
	}

	if (parse_command(line,&cmd,&arg) == -1) {
		printf("Error with parse command\n\r");
		return (-1);
	}

	while (p->cmd_string) {
		if (!strcmp(p->cmd_string, (char *)cmd)) {
			if (!p->cmd_function) {
				return (-1);
			}
			(*p->cmd_function)((char *)arg);
			success = 1;
			break;
		}
		p++;
	}

	if (success) {
		return (0);
	}
	else {
		return (-1);
	}
}

int get_command(uint8_t *command_buf) {
	static uint32_t counter=0;
	static uint32_t mode = COLLECT_CHARS;
	uint8_t ch = 0;;
	uint32_t mask;
	ch=dequeue(&queue);
	while (ch!=0)
	{
		if ((ch!='\n')&&(ch!='\r'))
		{
			if (ch==0x7f)
			{ // backspace functionality
				if (counter > 0)
				{
					printf("\b \b");
					counter--;
				}
			}
			else
			{
				putchar(ch); // send the character
				command_buf[counter++]=ch;
				if (counter>=(QUEUE_SIZE-2))
				{
					mode=COMPLETE;
					break;
				}
			}
		}
		else
		{
			mode = COMPLETE;
			break;
		}
		mask = disable();
		ch=dequeue(&queue);
		restore(mask);
	}

	if (mode == COMPLETE)
	{
		command_buf[counter] = 0;
		printf("\n\r");
		counter = 0;
		mode = COLLECT_CHARS;
		return(1);
	}
	else
	{
		return(0);
	}
}

void __attribute__((weak)) prompt(void) {
	printf("> ");
}

void __attribute__((weak)) help_command(char *arguments) {
	int i = 0;
	printf("Available Commands:\n\r");
	while(commands[i].cmd_string != 0)
	{
		printf("%s\n\r", commands[i].cmd_string);
		i++;
	}
	prompt();
}

void __attribute__((weak)) lof_command(char *arguments) {
	HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);
	prompt();
}

void __attribute__((weak)) lon_command(char *arguments) {
	HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
	prompt();
}

void __attribute__((weak)) test_command(char *arguments) {
	if (arguments != NULL) {
	    char *token = strtok(arguments, ",");
	    while (token != NULL) {
	    	printf("%s\n\r", token);
	        token = strtok(NULL, ",");
	    }
	    printf("OK\n\r");
	}
	prompt();
}
