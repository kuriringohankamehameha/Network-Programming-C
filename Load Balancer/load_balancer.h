#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sys/types.h>
#include<unistd.h>
#include<signal.h>
#include<fcntl.h>
#include<sys/wait.h>

typedef struct Node_t Node;

struct Node_t
{
    int pid;
    Node* rear;
    Node* front;
    Node* prev;
    Node* next;
};

Node* create_queue(int data)
{
    Node* head = (Node*) malloc (sizeof(Node));
    head->prev = NULL;
    head->next = NULL;
    head->pid = data;
    head->rear = head;
    head->front = head;
    return head;
}

Node* push(Node* head, int data)
{
    if (!head)
        return create_queue(data);

    Node* temp = head;
    Node* node = (Node*) malloc (sizeof(Node));
    node->pid = data;
    node->prev = NULL;
    node->next = temp;
    node->rear = node;
    node->front = head->front;
    temp->prev = node;
    head = node;
    return head;
}

void pop(Node* head)
{
    Node* temp = head->front;
    if (!head)
       return;
    
    Node* previous = temp->prev;

    if (previous)
    {
        previous->next = NULL;
        temp->prev = NULL;
        head->front = previous;
        
        free(temp);
        return;
    }

    else
    {
        head = NULL;
        free(temp);
        return;
    }
}

Node* shift_queue(Node* head)
{
    Node* temp = head->front;
    Node* temp2 = head->rear;
    Node* prev = temp->prev;
    prev->next = NULL;
    head->front = prev;
    temp->next = temp2;
    temp2->prev = temp;
    head->rear = temp;
    head = temp;

    return head;
}

void print_queue(Node* head)
{
    Node* temp = head;
    while(temp)
    {
        printf("PID: %d", temp->pid);
        if (temp->next == NULL)
        {
            printf("\n");
        }

        else
        {
            printf(" -> ");
        }

        temp = temp->next;
    }
}

