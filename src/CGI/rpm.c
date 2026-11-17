#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    FILE *file;
    char buffer[128];

    printf("Content-Type: application/json\r\n");
    printf("Access-Control-Allow-Origin: *\r\n");
    printf("\r\n");

    file = fopen("/tmp/can_data.json", "r");

    if (file == NULL)
    {
        printf("{\"error\":\"Dados CAN indisponiveis\"}\n");
        return 1;
    }

    if (fgets(buffer, sizeof(buffer), file) != NULL)
    {
        printf("%s", buffer);
    }
    else
    {
        printf("{\"error\":\"Arquivo vazio\"}\n");
    }

    fclose(file);

    return 0;
}
