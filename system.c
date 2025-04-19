#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>

#define PORT 8080
#define BUFFER_SIZE 1024

typedef struct
{
    int request_id;
    char operation[64];
    char parameters[256];
} Request;

typedef struct
{
    int request_id;
    int status_code;
    char result[512];
} Response;

int process_request(Request *req, Response *resp)
{
    resp->request_id = req->request_id;

    if (strcmp(req->operation, "CALCULATE") == 0)
    {
        int a, b;
        char operation;
        sscanf(req->parameters, "%d %c %d", &a, &operation, &b);

        resp->status_code = 200;

        switch (operation)
        {
        case '+':
            sprintf(resp->result, "%d", a + b);
            break;
        case '-':
            sprintf(resp->result, "%d", a - b);
            break;
        case '*':
            sprintf(resp->result, "%d", a * b);
            break;
        case '/':
            if (b == 0)
            {
                resp->status_code = 400;
                strcpy(resp->result, "Division by zero");
            }
            else
            {
                sprintf(resp->result, "%d", a / b);
            }
            break;
        default:
            resp->status_code = 400;
            strcpy(resp->result, "Unknown operation");
        }
    }
    else if (strcmp(req->operation, "ECHO") == 0)
    {
        resp->status_code = 200;
        strcpy(resp->result, req->parameters);
    }
    else
    {
        resp->status_code = 404;
        strcpy(resp->result, "Unknown operation");
    }

    return resp->status_code;
}

int main()
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    srand(time(NULL));

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Original system listening on port %d\n", PORT);

    while (1)
    {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        memset(buffer, 0, BUFFER_SIZE);
        int valread = read(new_socket, buffer, BUFFER_SIZE);

        if (valread > 0)
        {
            Request req;
            Response resp;

            memcpy(&req, buffer, sizeof(Request));
            process_request(&req, &resp);

            send(new_socket, &resp, sizeof(Response), 0);
            printf("Original: Processed request %d\n", req.request_id);
        }

        close(new_socket);
    }

    return 0;
}