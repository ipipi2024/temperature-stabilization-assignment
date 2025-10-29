#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <arpa/inet.h>
#include "utils.h"


int main (int argc, char *argv[])
{
    int socket_desc;
    struct sockaddr_in server_addr;
    char server_message[100], client_message[100];

    struct msg the_message; 
    
    // Command-line input arguments (user provided)
    int externalIndex = atoi(argv[1]); 
    float initialTemperature = atof(argv[2]); 

    // Current temperature (starts as initial, gets updated each iteration)
    float currentTemp = initialTemperature;

    // Create socket:
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);

    if(socket_desc < 0){
        printf("Unable to create socket\n");
        return -1;
    }

    printf("Socket created successfully\n");
    
    // Set port and IP the same as server-side:
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(2000);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        

    // Send connection request to server:
    if(connect(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        printf("Unable to connect\n");
        return -1;
    }
    printf("Connected with server successfully\n");
    printf("--------------------------------------------------------\n\n");

    // Send initial temperature to server
    the_message = prepare_message(externalIndex, currentTemp);

    if(send(socket_desc, (const void *)&the_message, sizeof(the_message), 0) < 0){
        printf("Unable to send initial message\n");
        return -1;
    }
    printf("Sent initial temperature: %f\n", currentTemp);

    // Loop to continuously communicate with server
    int iteration = 0;
    while (1) {
        iteration++;

        // Receive the server's response
        int byte = recv(socket_desc, (void *)&the_message, sizeof(the_message), 0);

        if (byte < 0) {
            printf("Error while receiving server's msg\n");
            return -1;
        }

        if (byte == 0) {
            printf("Server terminated unexpectedly\n");
            break;
        }

        // Check for termination signal (Index == -1)
        if (the_message.Index == -1) {
            printf("========================================\n");
            printf("CONVERGENCE ACHIEVED!\n");
            printf("Final temperature of External Process %d: %f\n", externalIndex, currentTemp);
            printf("========================================\n");
            break;
        }

        // Extract central temperature from server
        float centralTemp = the_message.T;
        printf("--------------------------------------------------------\n");
        printf("Iteration %d: Received central temperature = %f\n", iteration, centralTemp);

        // Update external temperature using the formula:
        // externalTemp = (3 * externalTemp + 2 * centralTemp) / 5
        currentTemp = (3.0 * currentTemp + 2.0 * centralTemp) / 5.0;
        printf("Iteration %d: Updated my temperature to = %f\n", iteration, currentTemp);

        // Send updated temperature back to server
        the_message = prepare_message(externalIndex, currentTemp);
        if (send(socket_desc, (const void *)&the_message, sizeof(the_message), 0) < 0) {
            printf("Unable to send updated temperature\n");
            return -1;
        }
    }

    // Close the socket:
    close(socket_desc);
    
    return 0;
}
