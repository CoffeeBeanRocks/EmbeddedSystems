#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <main.h>
#include <ctype.h>
#include "interrupt.h"
#include "queue.h"
#include "command.h"
#include "stm32l4xx_hal_rtc.h"

extern queue_t queue;
extern RTC_HandleTypeDef hrtc;

void help_command(char *arguments);
void lof_command(char *arguments);
void lon_command(char *arguments);
void test_command(char *arguments);
void ts_command(char *arguments);
void ds_command(char *arguments);

command_t commands[] = {
  {"help",help_command},
  {"lof",lof_command},
  {"lon",lon_command},
  {"test",test_command},
  {"ts",ts_command},
  {"ds",ds_command},
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
	RTC_TimeTypeDef current_time;
	RTC_DateTypeDef current_date;
	HAL_RTC_GetTime(&hrtc, &current_time, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &current_date, RTC_FORMAT_BIN);
	printf("%02d/%02d/20%02d ", current_date.Month, current_date.Date, current_date.Year);
	printf("%02d:%02d:%02d", current_time.Hours, current_time.Minutes, current_time.Seconds);
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

void __attribute__((weak)) ts_command(char *arguments) {

	int count = 0;
	char *str = arguments;
	while (*str) {
		if (*str == ',') {
			count++;
		}
		str++;
	}

	if(count != 2) {
		printf("NOK\n\r");
		return;
	}

	RTC_TimeTypeDef newTime = {0};

	char *num = strtok(arguments, ",");
	int result = atoi(num);
	if((!result && strcmp(num, "0")) || (result < 0 && result > 23))
	{
		printf("NOK\n\r");
		return;
	}

	newTime.Hours = strtol(num, NULL, 0);

	num = strtok(NULL, ",");
	result = atoi(num);
	if((!result && strcmp(num, "0")) || (result < 0 && result > 59))
	{
		printf("NOK\n\r");
		return;
	}
	newTime.Minutes = strtol(num, NULL, 0);

	num = strtok(NULL, ",");
	result = atoi(num);
	if((!result && strcmp(num, "0")) || (result < 0 && result > 59))
	{
		printf("NOK\n\r");
		return;
	}
	newTime.Seconds = strtol(num, NULL, 0);

	newTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
	newTime.StoreOperation = RTC_STOREOPERATION_RESET;
	if (HAL_RTC_SetTime(&hrtc, &newTime, RTC_FORMAT_BCD) != HAL_OK)
	{
		Error_Handler();
	}

	printf("OK\n\r");
	prompt();
}

int isLeapYear(int year) {
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
        return 1;
    } else {
        return 0;
    }
}

int isValidDate(int day, int month, int year) {
    if (year < 1 || year > 9999) {
        return 0;
    }

    if (month < 1 || month > 12) {
        return 0;
    }

    int maxDays = 31;

    if (month == 4 || month == 6 || month == 9 || month == 11) {
        maxDays = 30;
    } else if (month == 2) {
        if (isLeapYear(year)) {
            maxDays = 29;
        } else {
            maxDays = 28;
        }
    }

    if (day < 1 || day > maxDays) {
        return 0;
    }

    return 1;
}

uint8_t getMonth(int month) {
	if(month == 1) {
		return RTC_MONTH_JANUARY;
	}
	else if(month == 2) {
		return RTC_MONTH_FEBRUARY;
	}
	else if(month == 3) {
		return RTC_MONTH_MARCH;
	}
	else if(month == 4) {
		return RTC_MONTH_APRIL;
	}
	else if(month == 5) {
		return RTC_MONTH_MAY;
	}
	else if(month == 6) {
		return RTC_MONTH_JUNE;
	}
	else if(month == 7) {
		return RTC_MONTH_JULY;
	}
	else if(month == 8) {
		return RTC_MONTH_AUGUST;
	}
	else if(month == 9) {
		return RTC_MONTH_SEPTEMBER;
	}
	else if(month == 10) {
		return RTC_MONTH_OCTOBER;
	}
	else if(month == 11) {
		return RTC_MONTH_NOVEMBER;
	}
	else {
		return RTC_MONTH_DECEMBER;
	}
}

void __attribute__((weak)) ds_command(char *arguments) {

	int count = 0;
	char *str = arguments;
	while (*str) {
		if (*str == ',') {
			count++;
		}
		str++;
	}

	if(count != 2) {
		printf("NOK\n\r");
		return;
	}

	RTC_DateTypeDef newDate = {0};
	newDate.WeekDay = RTC_WEEKDAY_MONDAY;

	char *num = strtok(arguments, ",");
	int month = atoi(num);
	if(!month && strcmp(num, "0"))
	{
		printf("NOK\n\r");
		return;
	}

	int date = atoi(num);
	if(!date && strcmp(num, "0"))
	{
		printf("NOK\n\r");
		return;
	}

	int year = atoi(num);
	if(!year && strcmp(num, "0"))
	{
		printf("NOK\n\r");
		return;
	}

	if(!isValidDate(month, date, year))
	{
		printf("NOK\n\r");
		return;
	}

	newDate.Month = getMonth(month);
	newDate.Date = date;
	if(year == 0)
		newDate.Year = 0x0;
	else
		newDate.Year = year;

	if (HAL_RTC_SetDate(&hrtc, &newDate, RTC_FORMAT_BCD) != HAL_OK)
	{
		Error_Handler();
	}

	printf("OK\n\r");
	prompt();
}
