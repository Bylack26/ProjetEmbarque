#include "shell.h"




char line_buffer[MAX_LINE_SIZE];

void process_line(uint8_t uart_no, char* line){
    int* parameter;
    int command = get_command(line, parameter);
    if(command == 0){
        uart_send_string(uart_no, "Unknown Command");
    }else if(command == 1){
        uart_send_string(uart_no, line + *parameter + 1);
    }else if (command == 2){
        
    }
    return;
}

int get_command(char* line, int* parameter){
    int i = 0;
    while(i < MAX_LINE_SIZE && (line[i] != ' ' || line[i] != '\0')){
        i++;
    }
    line[i] = '\0';
    if(str_compare(line, "echo", i) == 0){
        *parameter = i;
        return 1;
    }else if(str_compare(line, "curs", i) == 0){
        *parameter = i;
        return 2;
    }else{
        *parameter = 0;
        return 0;
    }
    

}

int str_compare(char* first_str, char* compare_with, uint8_t first_size){
    int i = 0;
    while(i < first_size && first_str[i] != '\0' && compare_with[i] != '\0' && first_str[i] == compare_with[i]){
        i++;
    }
    if(i == first_size){
        return 0;
    }else{
        return 1;
    }
}

int get_parameter(char* line){
    int i =0;
    while(line[i] != '\0' || line[i] != '\n'){

    }
}