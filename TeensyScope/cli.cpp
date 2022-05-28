#include <arduino.h>
#include "cli.h"

#define CLI_MAX_PARAMS 4
#define CLI_BUF_SIZE 128

char cli_buf[CLI_BUF_SIZE];
int cli_buf_index;
char *cli_params[CLI_MAX_PARAMS];

extern cli_command_t cli_commands[];

void cli_process_command()
{
    int cmd;
    int param_cnt;

    if(cli_buf[0] == '\0') {
        return;
    }

    // Split command and parameters
    param_cnt = 0;
    for(int i=0; cli_buf[i]; i++) {
        if(cli_buf[i] == ' ') {
            cli_buf[i] = '\0';
            cli_params[param_cnt++] = &cli_buf[i+1];
        }
    }
    for(cmd=0; cli_commands[cmd].command[0]; cmd++) {
        if(strcmp(cli_commands[cmd].command, cli_buf) == 0) {
            cli_commands[cmd].func(param_cnt, cli_params);
            break;
        } 
    }
    if(cli_commands[cmd].command[0] == '\0') {
        Serial.println("Invalid command");
      }
      cli_buf_index = 0;
      cli_buf[0] = '\0';
}

void cli_loop()
{
    char c;

    if(Serial.available()) {
        c = Serial.read();
        c = tolower(c);
        switch(c) {
            case 0x08: // Backspace
                if(cli_buf_index > 0) {
                  cli_buf_index--;
                  cli_buf[cli_buf_index] = '\0';
                }
                break;
            case 0x0a: // LF
            case 0x0d: // CR
                cli_process_command();
                break;
            default:
                if(cli_buf_index < CLI_BUF_SIZE-1) {
                    cli_buf[cli_buf_index++] = c;
                    cli_buf[cli_buf_index] = '\0';
                }
                break;
        }
    }
}
