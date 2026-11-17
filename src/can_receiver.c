#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/ioctl.h>

#include <net/if.h>

#include <linux/can.h>
#include <linux/can/raw.h>

#define CAN_INTERFACE "can0"
#define CAN_ID_RPM     0x100

#define DATA_FILE      "/tmp/can_data.json"
#define TEMP_FILE      "/tmp/can_data.tmp"

int main(void)
{
    int socket_can;
    struct sockaddr_can address;
    struct ifreq interface;
    struct can_frame frame;

    struct can_filter filter;

    /* Cria o socket CAN */
    socket_can = socket(PF_CAN, SOCK_RAW, CAN_RAW);

    if (socket_can < 0)
    {
        perror("Erro ao criar socket CAN");
        return 1;
    }

    /* Identifica a interface can0 */
    memset(&interface, 0, sizeof(interface));

    strncpy(interface.ifr_name, CAN_INTERFACE,
            sizeof(interface.ifr_name) - 1);

    if (ioctl(socket_can, SIOCGIFINDEX, &interface) < 0)
    {
        perror("Erro ao identificar interface CAN");
        close(socket_can);
        return 1;
    }

    /* Configura filtro para receber somente o ID 0x100 */
    filter.can_id = CAN_ID_RPM;
    filter.can_mask = CAN_SFF_MASK | CAN_EFF_FLAG | CAN_RTR_FLAG;

    if (setsockopt(socket_can, SOL_CAN_RAW, CAN_RAW_FILTER,
                   &filter, sizeof(filter)) < 0)
    {
        perror("Erro ao configurar filtro CAN");
        close(socket_can);
        return 1;
    }

    /* Configura endereço do socket */
    memset(&address, 0, sizeof(address));

    address.can_family = AF_CAN;
    address.can_ifindex = interface.ifr_ifindex;

    /* Associa o socket à interface can0 */
    if (bind(socket_can, (struct sockaddr *)&address,
             sizeof(address)) < 0)
    {
        perror("Erro ao associar socket à interface CAN");
        close(socket_can);
        return 1;
    }

    printf("Receptor CAN iniciado!\n");
    printf("Interface: %s\n", CAN_INTERFACE);
    printf("ID monitorado: 0x%03X\n", CAN_ID_RPM);

    while (1)
    {
        ssize_t bytes_read;

        bytes_read = read(socket_can, &frame, sizeof(frame));

        if (bytes_read < 0)
        {
            if (errno == EINTR)
                continue;

            perror("Erro ao receber frame CAN");
            break;
        }

        if (bytes_read != sizeof(frame))
            continue;

        /* Confere se o frame possui dois bytes */
        if (frame.can_dlc != 2)
            continue;

        /* Decodifica RPM: byte mais significativo primeiro */
        uint16_t rpm = ((uint16_t)frame.data[0] << 8)
                       | frame.data[1];

        /* Salva o valor mais recente */
        FILE *file = fopen(TEMP_FILE, "w");

        if (file == NULL)
        {
            perror("Erro ao abrir arquivo temporário");
            continue;
        }

        fprintf(file, "{\"rpm\": %u}\n", rpm);

        fclose(file);

        /* Atualiza o arquivo de forma atômica */
        if (rename(TEMP_FILE, DATA_FILE) < 0)
        {
            perror("Erro ao atualizar arquivo de dados");
            continue;
        }

        printf("CAN ID: 0x%03X | RPM: %u\n",
               frame.can_id & CAN_SFF_MASK, rpm);

        fflush(stdout);
    }

    close(socket_can);

    return 0;
}
