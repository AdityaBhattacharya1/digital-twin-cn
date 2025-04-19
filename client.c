#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>

#define ORIGINAL_PORT 8080
#define TWIN_PORT 8081
#define BUFFER_SIZE 1024
#define SERVER_IP "127.0.0.1"

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

typedef struct
{
    int total_requests;
    int original_requests;
    int twin_requests;
    int successful_original;
    int successful_twin;
    float original_latency;
    float twin_latency;
} ABTestStats;

int send_request(const char *server_ip, int port, Request *req, Response *resp)
{
    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("Socket creation error\n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0)
    {
        printf("Invalid address/ Address not supported\n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("Connection Failed to port %d\n", port);
        return -1;
    }

    send(sock, req, sizeof(Request), 0);
    read(sock, resp, sizeof(Response));

    close(sock);
    return 0;
}

void print_ab_test_stats(ABTestStats *stats)
{
    printf("\n===== A/B Testing Statistics =====\n");
    printf("Total Requests: %d\n", stats->total_requests);
    printf("Original System Requests: %d (%.1f%%)\n",
           stats->original_requests,
           (float)stats->original_requests / stats->total_requests * 100);
    printf("Digital Twin Requests: %d (%.1f%%)\n",
           stats->twin_requests,
           (float)stats->twin_requests / stats->total_requests * 100);

    if (stats->original_requests > 0)
    {
        printf("Original Success Rate: %.1f%%\n",
               (float)stats->successful_original / stats->original_requests * 100);
        printf("Original Avg Latency: %.3f ms\n",
               stats->original_latency / stats->original_requests);
    }

    if (stats->twin_requests > 0)
    {
        printf("Digital Twin Success Rate: %.1f%%\n",
               (float)stats->successful_twin / stats->twin_requests * 100);
        printf("Digital Twin Avg Latency: %.3f ms\n",
               stats->twin_latency / stats->twin_requests);
    }
    printf("==================================\n\n");
}

int main()
{
    Request req;
    Response resp_original, resp_twin;
    ABTestStats stats = {0};
    struct timeval start, end;
    double time_taken;
    int next_req_id = 1;
    float ab_ratio = 0.3;

    srand(time(NULL));

    while (1)
    {
        printf("\nMenu:\n");
        printf("1. Send Calculate Request\n");
        printf("2. Send Echo Request\n");
        printf("3. Send Uppercase Request (Twin only)\n");
        printf("4. View A/B Test Stats\n");
        printf("5. Get Digital Twin Health Info\n");
        printf("6. Change A/B Test Ratio (current: %.1f)\n", ab_ratio);
        printf("7. Exit\n");
        printf("Enter choice: ");

        int choice;
        scanf("%d", &choice);
        getchar();

        switch (choice)
        {
        case 1:
        {
            strcpy(req.operation, "CALCULATE");
            printf("Enter calculation (e.g., 10 + 5): ");
            fgets(req.parameters, sizeof(req.parameters), stdin);
            req.parameters[strcspn(req.parameters, "\n")] = 0;
            break;
        }
        case 2:
        {
            strcpy(req.operation, "ECHO");
            printf("Enter text to echo: ");
            fgets(req.parameters, sizeof(req.parameters), stdin);
            req.parameters[strcspn(req.parameters, "\n")] = 0;
            break;
        }
        case 3:
        {
            strcpy(req.operation, "UPPERCASE");
            printf("Enter text to convert to uppercase: ");
            fgets(req.parameters, sizeof(req.parameters), stdin);
            req.parameters[strcspn(req.parameters, "\n")] = 0;

            req.request_id = next_req_id++;
            stats.total_requests++;
            stats.twin_requests++;

            gettimeofday(&start, NULL);
            if (send_request(SERVER_IP, TWIN_PORT, &req, &resp_twin) == 0)
            {
                gettimeofday(&end, NULL);
                time_taken = (end.tv_sec - start.tv_sec) * 1000.0 +
                             (end.tv_usec - start.tv_usec) / 1000.0;
                stats.twin_latency += time_taken;

                if (resp_twin.status_code == 200)
                {
                    stats.successful_twin++;
                }

                printf("\nResponse from Digital Twin:\n");
                printf("Status: %d\n", resp_twin.status_code);
                printf("Result: %s\n", resp_twin.result);
                printf("Latency: %.3f ms\n", time_taken);
            }

            continue;
        }
        case 4:
            print_ab_test_stats(&stats);
            continue;
        case 5:
        {
            strcpy(req.operation, "HEALTH");
            strcpy(req.parameters, "");
            req.request_id = next_req_id++;
            stats.total_requests++;
            stats.twin_requests++;

            gettimeofday(&start, NULL);
            if (send_request(SERVER_IP, TWIN_PORT, &req, &resp_twin) == 0)
            {
                gettimeofday(&end, NULL);
                time_taken = (end.tv_sec - start.tv_sec) * 1000.0 +
                             (end.tv_usec - start.tv_usec) / 1000.0;
                stats.twin_latency += time_taken;

                if (resp_twin.status_code == 200)
                {
                    stats.successful_twin++;
                }

                printf("\nHealth Info from Digital Twin:\n");
                printf("%s\n", resp_twin.result);
            }

            continue;
        }
        case 6:
            printf("Enter new A/B test ratio (0.0-1.0): ");
            scanf("%f", &ab_ratio);
            if (ab_ratio < 0.0)
                ab_ratio = 0.0;
            if (ab_ratio > 1.0)
                ab_ratio = 1.0;
            printf("A/B test ratio updated to %.1f\n", ab_ratio);
            continue;
        case 7:
            return 0;
        default:
            printf("Invalid choice\n");
            continue;
        }

        req.request_id = next_req_id++;
        stats.total_requests++;

        int use_twin = ((float)rand() / RAND_MAX) < ab_ratio;

        if (use_twin)
        {
            stats.twin_requests++;
            gettimeofday(&start, NULL);

            if (send_request(SERVER_IP, TWIN_PORT, &req, &resp_twin) == 0)
            {
                gettimeofday(&end, NULL);
                time_taken = (end.tv_sec - start.tv_sec) * 1000.0 +
                             (end.tv_usec - start.tv_usec) / 1000.0;
                stats.twin_latency += time_taken;

                if (resp_twin.status_code == 200)
                {
                    stats.successful_twin++;
                }

                printf("\nResponse from Digital Twin:\n");
                printf("Status: %d\n", resp_twin.status_code);
                printf("Result: %s\n", resp_twin.result);
                printf("Latency: %.3f ms\n", time_taken);
            }
        }
        else
        {
            stats.original_requests++;
            gettimeofday(&start, NULL);

            if (send_request(SERVER_IP, ORIGINAL_PORT, &req, &resp_original) == 0)
            {
                gettimeofday(&end, NULL);
                time_taken = (end.tv_sec - start.tv_sec) * 1000.0 +
                             (end.tv_usec - start.tv_usec) / 1000.0;
                stats.original_latency += time_taken;

                if (resp_original.status_code == 200)
                {
                    stats.successful_original++;
                }

                printf("\nResponse from Original System:\n");
                printf("Status: %d\n", resp_original.status_code);
                printf("Result: %s\n", resp_original.result);
                printf("Latency: %.3f ms\n", time_taken);
            }
        }
    }

    return 0;
}