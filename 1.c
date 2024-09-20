#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>

#define int long long

int N, ssize;
key_t s_key, m_key;

struct MessageBuffer
{
    int mtype;
    int key;
};

struct HashNode
{
    char *word;
    int freq;
    struct HashNode *next;
};

struct HashNode *hashTable[100003] = {NULL};

int hash(char *str)
{
    int hash = 0;
    int c;
    while (*str != '\0')
    {
        c = *str;
        *str++;
        hash = (hash * 101 + c) % 100003;
    }
    return hash;
}

void insert(char *word)
{
    int i = hash(word);
    struct HashNode *node = hashTable[i];

    while (node != NULL)
    {
        if (strcmp(node->word, word) == 0)
        {
            node->freq++;
            return;
        }
        node = node->next;
    }

    struct HashNode *newNode = (struct HashNode *)malloc(sizeof(struct HashNode));
    newNode->word = (char *)malloc((ssize + 1) * sizeof(char));
    for (int i = 0; word[i] != '\0'; i++)
    {
        newNode->word[i] = word[i];
    }
    newNode->word[strlen(word)] = '\0';
    newNode->freq = 1;
    newNode->next = hashTable[i];
    hashTable[i] = newNode;
}

int occ_count(char *word)
{
    int i = hash(word);
    struct HashNode *node = hashTable[i];
    while (node != NULL)
    {
        if (strcmp(node->word, word) == 0)
        {
            return node->freq;
        }
        node = node->next;
    }
    return 0;
}

void create_hashTable(FILE *file)
{
    char buffer[ssize];
    for (; fscanf(file, "%s", buffer) != EOF;)
    {
        insert(buffer);
    }
}

void freetable()
{
    for (int i = 0; i < 100003; i++)
    {
        struct HashNode *node = hashTable[i];
        while (node != NULL)
        {
            struct HashNode *temp = node;
            node = node->next;
            free(temp->word);
            free(temp);
        }
    }
}

signed main(int argc, char *argv[])
{
    int t = atoi(argv[1]);

    char input_file[11], words_file[11];
    sprintf(input_file, "input%d.txt", t);
    sprintf(words_file, "words%d.txt", t);

    FILE *in_ptr = fopen(input_file, "r");

    fscanf(in_ptr, "%d %d %d %d", &N, &ssize, &s_key, &m_key);
    fclose(in_ptr);

    FILE *word_ptr = fopen(words_file, "r");
    create_hashTable(word_ptr);
    fclose(word_ptr);

    int shm_id = shmget(s_key, sizeof(char[N][N][ssize]), 0666);
    char(*shmptr)[N][ssize] = shmat(shm_id, NULL, 0);
    int msgq_id = msgget(m_key, 0666);

    for (int i = 0; i < 2 * N - 1; i++)
    {
        int count = 0;
        struct MessageBuffer msg_r;
        int d_key;

        if (i > 0)
        {
            msgrcv(msgq_id, &msg_r, sizeof(struct MessageBuffer) - sizeof(int), 2, 0);
            d_key = msg_r.key;
        }

        for (int x = i, y = 0; y <= i; y++, x--)
        {
            if (x >= 0 && x < N && y >= 0 && y < N)
            {
                if (i > 0)
                {
                    for (int z = 0; shmptr[x][y][z] != '\0' && z < ssize; z++)
                    {
                        shmptr[x][y][z] = (shmptr[x][y][z] - 'a' + d_key) % 26 + 'a';
                    }
                }
                count += occ_count(shmptr[x][y]);
            }
        }

        struct MessageBuffer msg_s;
        msg_s.mtype = 1;
        msg_s.key = count;
        msgsnd(msgq_id, &msg_s, sizeof(struct MessageBuffer) - sizeof(int), 0);
    }
    freetable();
    shmdt(shmptr);
    shmctl(shm_id, IPC_RMID, NULL);
    msgctl(msgq_id, IPC_RMID, NULL);

    return 0;
}