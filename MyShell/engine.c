#include "shell.h"
#include <ncurses.h>

#define gotoxy(x, y) printf("\033[%d;%dH", (y), (x))
#define ALT_BACKSPACE 127

typedef struct Node_t Node;

struct Node_t{
    char data;
    Node* child[26];
    int isEnd;
};

char alphabets[] = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z'};

static int a = 0;
static int b = 0;
static int completion_count = 0;

/* Reference for our ncurses Window  and x,y coordinates of it's cursor */
WINDOW* win;
int x, y;

void backspace()
{
    /* Set No Echo mode */
    noecho();
    nocbreak();
    
    /* Goto current cursor position and delete the previous character */
    getyx(win, y, x);
    move(y, x-1);
    delch();
    
    cbreak();
    refresh();
}

Node* newNode()
{
    Node* head = (Node*) malloc (sizeof(Node));
    head->isEnd = 0;
    
    for(int i=0; i<26; i++)
        head->child[i] = NULL;
    return head;
}

Node* insertTrie(Node* head, char* str)
{
    /* Inserts the String into the Trie */
    Node* temp = head;
    for (int i=0; str[i] != '\0'; i++)
    {
        int position = str[i] - 'a';
        
        if (temp->child[position] == NULL)
            temp->child[position] = newNode();

        temp = temp->child[position];
    }
    temp->isEnd = 1;
    return head;
}

int searchTrie(Node* head, char* str)
{
    Node* temp = head;

    for(int i=0; str[i]!='\0'; i++)
    {
        int position = str[i] - 'a';
        if (temp->child[position] == NULL)
            return 0;
        temp = temp->child[position];
    }

    if (temp != NULL && temp->isEnd == 1)
        return 1;
    return 0;
}

char* str_concat(char* s1, char s2)
{
    char* op = (char*) malloc (sizeof(char));
    int k = 0;
    for(int i=0; i<strlen(s1)+1; i++)
    {
        if (i < strlen(s1))
            op[k++] = s1[i];
        else
            op[k++] = s2;
    }
    op[k] = '\0';
    return op;
}

void recursiveInsert(Node* temp, char** matches, int start, char* op)
{
    if (start >= 26)
        return;
    if(temp){
        if (temp->isEnd)
        {
            printw("%s\n", op);
            completion_count ++;
        }
        for(int i=start; i<26; i++)
        {
            if (temp->child[i])
            {
                Node* match_child;
                for (int j=0; j<26; j++)
                {
                    match_child = temp->child[j];
                    recursiveInsert(match_child, matches, 0, str_concat(op, alphabets[j]));
                }
                break;
            }
        }
        a = 0;
        b = 0;
    }
}

int completeWord(Node* head, char* str, char** matches, char* op)
{
    Node* temp = head;
    for(int i=0; str[i]!='\0'; i++)
    {
        int position = str[i] - 'a';
        if (temp->child[position] == NULL)
            return 0;
        temp = temp->child[position];
    }

    if (temp != NULL)
    {
        printw("\n");
        recursiveInsert(temp, matches, 0, op);
        return 1;
    }   
    return 0;
}

void clear_complete()
{
    /* Clears the previous autocomplete words from the Screen */
    if (completion_count == 0)
        return;
    for(int i=0; i<completion_count; i++)
    {
        move(y+1+i, 0);
        clrtoeol();
    }
    move(y, x);
}

int main()
{
    win = initscr();
    cbreak();
    noecho();

    /* Engine Code here */
    char* samples[] = {"thesis","omg", "theresa", "thevenin","lol"};
    int n = sizeof(samples)/sizeof(samples[0]);
    
    char** matches = (char**) malloc (100 * sizeof(char*));
    for(int i=0; i<100; i++)
        matches[i] = (char*) malloc (sizeof(char));

    Node* root = newNode();
    for(int i=0; i<n; i++)
        insertTrie(root, samples[i]);

    char* op = (char*) malloc (sizeof(char));
    //searchTrie(root, "the") ? printf("Yes\n") : printf("No\n");
    //searchTrie(root, "theresa") ? printf("Yes\n") : printf("No\n");
    //completeWord(root, "th", matches, "th") ? printf("Yes\n") : printf("No\n");

    int ch;
    int k = 0;
    while( (ch = getch()) != '\n' )
    {
        if (ch == '\t')
        {
            op[k] = '\0';
            getyx(win, y, x);

            /* Clear the previous completion words */
            clear_complete();
            
            completeWord(root, op, matches, op);
            
            completion_count = 0;
            move(y, x);
            clrtoeol();
            
            strcpy(op, "0");
            k = 0;
        }   
        else if (ch == ALT_BACKSPACE)
        {
            /* Check for backspace key (ch == 127) */
            backspace();
        }
        else
        {
            printw("%c", ch);
            op[k++] = ch; 
        }       
    }
    op[k] = '\0';

    endwin();
    return 0;
}
