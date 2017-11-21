//
//  controller.c
//  
//
//  Created by Laura Adams on 11/18/16.
//
//

#include <stdio.h>
// List of includes
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <inttypes.h>
#include <stdbool.h>
#include <unistd.h>

#define MAXBUF 1024
#define MAXPORT 6
#define MAXHOSTNAME 200
#define MAXACTION  10
#define MAXRESPONSE  20
#define MAXACTIVEMEM 20
#define MAXLISTEN 5
#define filename "log.txt"
#define fName "list.txt"

struct member{
    char agent[MAXHOSTNAME];
    unsigned long long time;
    struct member *nextMember;
};

int main (int argc, char *argv[]){

    int sockfd, networkSockfd, portNo, numBytesRead, numBytesWrite;
    pid_t childpid;
    socklen_t agentLEN;
    
    char buffer[MAXBUF];
    char cPort[MAXPORT];
    char cResponse [MAXRESPONSE];
    char cAction [MAXACTION];
    char cServerIP[MAXHOSTNAME];
    char cAgentIP[MAXHOSTNAME];
    
    struct sockaddr_in controler_addr, agent_addr;
    
    //pointer to log file
    FILE *fptr;
    FILE *fptrList;
    unsigned long fileSize;
    
    //variable for time stamp
    struct timeval timestamp;
    unsigned long long timeAction;
    unsigned long long timeResponse;
    
    //need a list for active members
    struct member *memberList;
    memberList = NULL;

    int memberCounter = 0;
    
    //flag if member is active
    bool isMember;
    
    //inputing port #
    if (argc < 2)
    {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }
    
    //creates socket discriptor for server and checks to make sure it works
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
            fprintf(stderr,"ERROR, socket() failed\n");
    }
    
    //initialize portNo to zero, and then sets it with input
    memset(cPort, 0, MAXPORT);
    sprintf(cPort,"%s",argv[1]);
    portNo = atoi(cPort);
    
    //sets stuct elements for bind()
    controler_addr.sin_family = AF_INET;
    controler_addr.sin_addr.s_addr = INADDR_ANY;
    controler_addr.sin_port = htons(portNo);
    
    //clear cServer
    memset(cServerIP, 0, MAXHOSTNAME);
    
    //converts addr into text string (controller's addr)
    inet_ntop(AF_INET, &controler_addr.sin_addr.s_addr, cServerIP, sizeof(controler_addr));
    printf("Server: %s \n", cServerIP);
    
    //binds socket to port and checks to make sure it works properly
    if (bind(sockfd, (struct sockaddr *) &controler_addr,sizeof(controler_addr)) < 0)
    {
        fprintf(stderr,"ERROR, bind() failed\n");
    }

    //creates listening queue that is 5 elements
    listen(sockfd,MAXLISTEN);
    
    printf("Listening on ip %s and port %d\n", cServerIP, ntohs(controler_addr.sin_port));

    //uses accept()
    while (true)
    {
        agentLEN = sizeof(agent_addr);
        memset(cAgentIP, 0, MAXHOSTNAME);
        memset(cResponse, 0, MAXRESPONSE);
        memset(buffer, 0, MAXBUF);
        
        //flag if member is active
        bool isMember = false;
        
        //open log.file
        if ((fptr = fopen(filename,"a")) == NULL)
        {
            printf("Error in opening file");
            exit(0);
        }
        else
        {
            fseek(fptr, 0, SEEK_END);
            fileSize = ftell(fptr);
            rewind(fptr);
        }
        
        if ((networkSockfd = accept(sockfd, (struct sockaddr *) &agent_addr, &agentLEN)) < 0)
        {
            fprintf(stderr,"ERROR, accpet() failed\n");
        }
        
        //converts addr into text string (agent's addr)
        inet_ntop(AF_INET, &agent_addr.sin_addr.s_addr, cAgentIP, sizeof(agent_addr));
        
        //reading from agent
        if ( (numBytesRead = read(networkSockfd, buffer, MAXBUF)) < 0)
        {
            fprintf(stderr,"ERROR, in reading data from agent \n");
        }
        
        //get time stamp of action
        gettimeofday(&timestamp, NULL);
        //converts time into milliseconds
        timeAction = (unsigned long long)(timestamp.tv_sec)*1000 + (unsigned long long)(timestamp.tv_usec)/1000;
        
        //setting action to hold request from agent
        memset(cAction,0,MAXACTION);
        sprintf(cAction,"%s",buffer);
        
        //writes to log file: agent wants action in buffer
        fprintf(fptr,"%llu : recieved a %s action from agent %s\n", timeAction, cAction, cAgentIP);
        
        //checks if agent is active
        struct member *traverse;
        traverse = memberList;
        while (traverse != NULL && isMember == false)
        {
            if ((strncmp(traverse->agent, cAgentIP, MAXHOSTNAME)) == 0)
            {
                isMember = true;
            }
            traverse = traverse->nextMember;
        }
        
        //conditions to action of agent -> #JOIN, #LEAVE, #LIST, #LOG
        
        if ((strncmp(cAction, "#JOIN", MAXACTION)) == 0)
        {
            gettimeofday(&timestamp, NULL);
            timeResponse = (unsigned long long)(timestamp.tv_sec)*1000 + (unsigned long long)(timestamp.tv_usec)/10000;
            
            //check list if member is active
                //if member is active -> respond with "$ALREADY MEMBER"
            if( isMember == true)
            {
                //sent to agent
                sprintf(cResponse,"$ALREADY MEMBER");
                numBytesWrite = write (networkSockfd, cResponse, sizeof(cResponse));
                
                //sent to log file
                fprintf(fptr,"%llu : Responded to agent %s with %s\n",
                        timeResponse, cAgentIP, cResponse);
            }
            
            //else -> respond with "$OK"
            else
            {
                //sent to agent
                sprintf(cResponse,"$OK");
                numBytesWrite = write (networkSockfd, cResponse, strlen(cResponse));
        
                if (memberList == NULL){
                    memberList = malloc( sizeof(struct member) );
                    strncpy(memberList->agent, cAgentIP, MAXHOSTNAME);
                    memberList->time = timeResponse;
                    memberList->nextMember = NULL;
                }
                
                else //( pointer != NULL )
                {
                    struct member *pointer1;
                    struct member *pointer2;
                    
                    //set second pointer to head of list, to prevent dereferencing
                    pointer2 = memberList;
                    
                    /* Creates a node at the beginning of the list */
                    pointer1 = malloc( sizeof(struct member) );

                    //initialize the new member
                    strncpy(pointer1->agent, cAgentIP, MAXHOSTNAME);
                    pointer1->time = timeResponse;
                    pointer1->nextMember = pointer2;
                    memberList = pointer1;

                }
                
                memberCounter++;
                
                //sent to log file
                fprintf(fptr,"%llu : Responded to agent %s with %s\n",
                        timeResponse, cAgentIP, cResponse);
            }
            fclose(fptr);
        }
        
        if ((strncmp(cAction, "#LEAVE", MAXACTION)) == 0)
        {
            gettimeofday(&timestamp, NULL);
            timeResponse = (unsigned long long)(timestamp.tv_sec)*1000 + (unsigned long long)(timestamp.tv_usec)/10000;
            
            //check list if member is active
            //if member is active -> respond with "$OK"
            if(isMember == true)
            {
                //sent to agent
                sprintf(cResponse,"$OK");
                numBytesWrite = write (networkSockfd, cResponse, strlen(cResponse));
                
                //sent to log file
                fprintf(fptr,"%llu : Responded to agent %s with %s\n",
                        timeResponse, cAgentIP, cResponse);
 
                //traverse list to find member
                struct member *pointer1;
                struct member *pointer2;
                
                //to prevent dereferencing of the list
                pointer1 = memberList;
                
                //case1: if member is at the start of the list
                if((strncmp(pointer1->agent, cAgentIP, MAXHOSTNAME)) == 0)
                {
                    memberList = memberList->nextMember;
                    free(pointer1); //deletes member from active list
                }
                
                else
                {
                    bool delete = false;
                    pointer2 = memberList->nextMember;
                    
                    //case2: if member is in the middle of the list
                    while(pointer2 != NULL && pointer1 != NULL && delete == false)
                    {
                        if((strncmp(pointer2->agent, cAgentIP, MAXHOSTNAME)) == 0)
                        {
                            //printf("found in the middle\n");
                            struct member *temp;
                            temp = pointer2;
                            pointer2 = pointer2->nextMember;
                            pointer1->nextMember = pointer2;
                            free(temp);
                            delete = true;
                        }
                        else
                        {
                            //moves pointers along list
                            pointer2 = pointer2->nextMember;
                            pointer1 = pointer1->nextMember;
                        }
                    }
                    //case3: if member is at the end of the list
                    if(pointer1 != NULL && pointer2 == NULL)
                    {
                        //printf("found at end\n");
                        if((strncmp(pointer1->agent, cAgentIP, MAXHOSTNAME)) == 0)
                        {
                            free(pointer1);
                        }
                    }
                }
                memberCounter--;
            }
            
            //else -> respond with "$NOT MEMBER"
            else
            {
                //sent to agent
                sprintf(cResponse,"$NOT MEMBER");
                numBytesWrite = write (networkSockfd, cResponse, strlen(cResponse));
                
                //sent to log file
                fprintf(fptr,"%llu : Responded to agent %s with %s\n",
                        timeResponse, cAgentIP, cResponse);
            }
            fclose(fptr);
        }
        
        if ((strncmp(cAction, "#LIST", MAXACTION)) == 0)
        {
            gettimeofday(&timestamp, NULL);
            timeResponse = (unsigned long long)(timestamp.tv_sec)*1000 + (unsigned long long)(timestamp.tv_usec)/10000;;
            
            //check list if member is active
            //if member is active -> respond with sending list to member
            if(isMember == true)
            {
                //send copy of active member list to agent
                
                //clear buffer
                memset(buffer, 0, MAXBUF);
                
                struct member *traverse1;
                traverse1 = memberList;
                char tempAgent[MAXHOSTNAME];
                unsigned long long tempTime;
                memset(tempAgent, 0, MAXHOSTNAME);
                
                //writing to list.txt
                if ((fptrList = fopen(fName,"w")) == NULL)
                {
                    printf("Error in opening file");
                    exit(0);
                }
                
                while (traverse1 != NULL)
                {
                    //getting values of active member list
                    strncpy(tempAgent, traverse1->agent, MAXHOSTNAME);
                    tempTime = timeResponse - traverse1->time;
                    
                    //sending these values to list.txt
                    fprintf(fptrList,"%s %llu\n", tempAgent, tempTime);
                    traverse1 = traverse1->nextMember;
                }
                fclose(fptrList);
                
                //read file into buffer
                fptrList = fopen(fName,"r");
                
                memset(buffer, 0, MAXBUF);
                numBytesRead = fread(buffer, sizeof(char), sizeof(buffer), fptrList);
                if(numBytesRead == 0) break;
                if(numBytesRead < 0)
                {
                    printf("Error in reading list.txt to buffer\n");
                }
                
                void *p = buffer;
                //send list file pieces at a time
                while (numBytesRead > 0)
                {
                    numBytesWrite = write(networkSockfd, buffer, numBytesRead);
                    if(numBytesWrite <=0)
                    {
                        printf("Error in writing list.txt to buffer");
                    }
                    
                    //keep track of how many bytes needed to write
                    numBytesRead -= numBytesWrite;
                    //keeps track of where in the buffer the pointer is
                    p += numBytesWrite;
                    
                }
                
                sprintf(cResponse,"$OK");
                fprintf(fptr,"%llu : Responded to agent %s with list (%s)\n",
                        timeResponse, cAgentIP, cResponse);
                
            }
            
            //else -> ignore request
            else
            {
                //sent to agent
                sprintf(cResponse,"$NOT MEMBER");
                numBytesWrite = write (networkSockfd, cResponse, strlen(cResponse));
                
                //sent to log file
                fprintf(fptr,"%llu : No response is supplied to agent %s ( %s )\n",
                        timeResponse, cAgentIP, cResponse);
            }
            
            fclose(fptr);
        }
        
        if ((strncmp(cAction, "#LOG", MAXACTION)) == 0)
        {
            gettimeofday(&timestamp, NULL);
            timeResponse = (unsigned long long)(timestamp.tv_sec)*1000 + (unsigned long long)(timestamp.tv_usec)/10000;
            //check list if member is active
            //if member is  not active -> ignore request
            
            if (isMember == false)
            {
                //printf("LOG: is not a member\n");
                //sent to agent
                sprintf(cResponse,"$NOT MEMBER");
                numBytesWrite = write (networkSockfd, cResponse, strlen(cResponse));
                
                //sent to log file
                fprintf(fptr,"%llu : No response is supplied to agent %s ( %s )\n",
                        timeResponse, cAgentIP, cResponse);
                
                fclose(fptr);
                
            }
            
            //else -> respond with sending 'log.txt' to member
            else
            {
                memset(buffer, 0, MAXBUF);
                
                sprintf(cResponse,"$OK");
                fprintf(fptr,"%llu : supplied log file to agent %s ( %s )\n",
                        timeResponse, cAgentIP, cResponse);

                fclose(fptr);
                fptr = fopen(filename,"r");
                //read file into buffer
                numBytesRead = fread(buffer, sizeof(char), sizeof(buffer), fptr);
                if(numBytesRead == 0) break;
                if(numBytesRead < 0)
                {
                    printf("Error in reading log.txt to buffer\n");
                }
                
                void *p = buffer;
                //send log file pieces at a time
                while (numBytesRead > 0)
                {
                    numBytesWrite = write(networkSockfd, buffer, numBytesRead);
                    if(numBytesWrite <=0)
                    {
                        printf("Error in writing log.txt to buffer");
                    }
                    
                    //keep track of how many bytes needed to write
                    numBytesRead -= numBytesWrite;
                    //keeps track of where in the buffer the pointer is
                    p += numBytesWrite;
                
                }
                
            }
        }
        close(networkSockfd);
        

    }//end of while(true) loop
    
}
