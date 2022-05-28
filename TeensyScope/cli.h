typedef struct cli_command_s
{
    char command[10];
    void (*func)(int, char**);
} cli_command_t;

void cli_loop();
