#include "linked_list.h"

LinkedList* allocate_list (int data) {
    // Allocates memory for a Linkedlist pointer
    LinkedList* list = (LinkedList*) calloc (1, sizeof(LinkedList));
    list->acked = 0;
    list->data = data;
    return list;
}

LinkedList* linkedlist_insert(LinkedList* list, int item) {
    // Inserts the item onto the Linked List
    if (!list) {
        LinkedList* head = allocate_list(item);
        head->next = NULL;
        list = head;
        return list;
    } 
    
    else if (list->next == NULL) {
        LinkedList* node = allocate_list(item);
        node->next = NULL;
        list->next = node;
        return list;
    }

    LinkedList* temp = list;
    while (temp->next) {
        temp = temp->next;
    }
    
    LinkedList* node = allocate_list(item);
    node->next = NULL;
    temp->next = node;
    
    return list;
}

inline void free_data(void* data) {
    // Free anything related to data
    // You must implement this as per your convencience
    fprintf(stderr, "Warning: You must free the memory related to data in the function free_data(void* data)\n");
    fprintf(stderr, "Warning: Default Behavior will be to just free the pointer to the data\n");
}

LinkedList* linkedlist_remove(LinkedList* list) {
    // Removes the head from the linked list
    if (!list)
        return NULL;
    if (!(list->next))
        return NULL;
    LinkedList* node = list->next;
    LinkedList* temp = list;
    temp->next = NULL;
    list = node;
    //void* it = NULL;
    //memcpy(temp->data, it, sizeof(*(temp->data)));
    free(temp);
    return list;
}

void free_linkedlist(LinkedList* list) {
    LinkedList* temp = list;
    if (!list)
        return;
    while (list) {
        temp = list;
        list = list->next;
        free(temp);
    }
}

inline void print_data(LinkedList* node) {
    printf("%d", node->data);
}

void print_linkedlist(LinkedList* head) {
    for (LinkedList* node = head; node != NULL; node = node->next) {
        print_data(node);
        printf(" -> ");
    }
    printf("\n");
}

/*
int main() {
    LinkedList* head = allocate_list(50);
    int arr[] = {100, 200, 300, 400};
    for (int i=0; i<4; i++)
        head = linkedlist_insert(head, arr[i]);
    print_linkedlist(head);
    head = linkedlist_remove(head);
    head = linkedlist_remove(head);
    print_linkedlist(head);
    free_linkedlist(head);
    return 0;
}
*/
