#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

#define PERMS 0666
#define MAXBUFF 4096

//trie
typedef struct Node{
	char letter;
	char *leaf;
	struct Node *next_level;
	struct Node *same_level;
	struct Node1 *eoString;
}trie;

typedef struct Node1{
	int lineNum;
	char *path;
	struct Node1 *next;
}posting_list;

void insertion(struct Node **,int ,char *,char *);
struct Node *init(void);
struct Node1 *pl_init(void);
void destroyNodes(trie **,int );

//jobExecutor
int countLines(char *);
int mSL(char *,int );

//connections
long proConn(int *,char **,int ,int );
int execQueries(long *,int ,char **,int ,double ,char **,int *,int );
int digs(long );
char *godFather(long ,int ,int );
char *newPathName(char *,char *);
int mapping(char *,char ***,int **);
int str_split(char *,char ***);
void handler(int );
static int resume;
static int endProc;
void caller(struct Node *,char **,int ,int ,int ,int );
int alphaGreater(char *,char *);
char cwd[1024];
void writeLog(long ,char *,char *,char *,int );

//searchMode
void search(struct Node *,char **,int );
void writeSearchFifo(struct Node1 *,int );

//counts
void maxmin_Count(struct Node *,char *,int ,int );
void writeCountFifo(int ,posting_list *,int ,int ,char *);

typedef struct Node2{
	int word_freq;
	int char_freq;
	int line_freq;
}wc;

void wordCount(struct Node2 *,struct Node *,int );
struct Node2 wc_init(int ,int );
void writeWcFifo(struct Node2);