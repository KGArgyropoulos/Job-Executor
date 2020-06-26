#include "jobExecutor.h"

void maxmin_Count(trie *root,char *keyword,int fileCounter,int flag){

	int writefd;
	int digits=digs(getpid());
	char *wrName=godFather(getpid(),digits,0);
	if((writefd = open(wrName, O_WRONLY | O_APPEND) ) < 0){
		perror("worker: can't open write fifo");
		exit(1);
	}

	trie *temp=root->next_level;
	int	i=0;
	while(i<strlen(keyword)){
		if(temp->letter==keyword[i]){
			i++;
			if(i==strlen(keyword)){
				//found
				if(temp->eoString){
					posting_list *pl=temp->eoString;
					writeCountFifo(flag,pl,writefd,fileCounter,keyword);
				}
			}
			else if(temp->next_level){
				temp=temp->next_level;
			}
			else{
				posting_list *pl=NULL;
				break;
			}
		}
		else{
			if(temp->same_level){
				temp=temp->same_level;
			}
			else{
				posting_list *pl=NULL;
				break;
			}
		}
	}
	close(writefd);
	free(wrName);
}

void writeCountFifo(int flag,posting_list *pl,int writefd,int fileCounter,char *keyword){

	int res[fileCounter];
	int i,counter=0;
	res[0]=1;
	for(i=1;i<fileCounter;i++)
		res[i]=0;

	char **pathNames=malloc(fileCounter*sizeof(char *));
	pathNames[0]=malloc(((int)strlen(pl->path)+1)*sizeof(char ));
	strcpy(pathNames[0],pl->path);

	int val=1;
	int val_ptr=0;

	while(pl->next){
		posting_list *temp=pl->next;
		if(strcmp(temp->path,pl->path)==0){
			res[counter]++;
			if(flag==0){
				if(res[counter]>val){
					val=res[counter];
					val_ptr=counter;
				}
			}
			else{
				if(counter==val_ptr){
					val++;
				}
			}
		}
		else{
			if(flag==1){
				if(res[counter]<val){
					val=res[counter];
					val_ptr=counter;
				}
			}
			counter++;
			pathNames[counter]=malloc(((int)strlen(temp->path)+1)*sizeof(char ));
			strcpy(pathNames[counter],temp->path);
			res[counter]++;
		}
		pl=pl->next;
	}

	for(i=0;i<fileCounter;i++){
		if(res[i]==0){
			pathNames[i]=malloc(6*sizeof(char ));
			strcpy(pathNames[i],"EMPTY");
		}
	}

	int digits=digs(val);
	int total_length=((int)strlen(pathNames[val_ptr])+digits+3);
	char finalString[total_length];
	sprintf(finalString,"%s\n%d\n",pathNames[val_ptr],val);

	if(write(writefd,finalString,total_length) != total_length){
		perror("worker: can't write fifo");
		exit(1);
	}
	///////////////////////////////////////////////
	char type[9];
	if(flag==0)
		strcpy(type,"maxcount");
	else
		strcpy(type,"mincount");
	writeLog((long)getpid(),type,keyword,pathNames[val_ptr],0);
	///////////////////////////////////////////////

	for(i=0;i<fileCounter;i++)
		free(pathNames[i]);
	free(pathNames);

}

void wordCount(wc *wcount,trie *root,int flag){

	trie *temp;
	if(flag==0)
		temp=root->next_level;
	else
		temp=root->same_level;

	if(temp->eoString){
		posting_list *pl=temp->eoString;
		(*wcount).word_freq++;
		(*wcount).char_freq+=strlen(temp->leaf);
		while(pl->next){
			posting_list *pl1=pl;
			pl=pl->next;
			(*wcount).word_freq++;
			(*wcount).char_freq+=strlen(temp->leaf);
		}
	}
	if(temp->next_level){
		wordCount(wcount,temp,0);
		if(temp->same_level){
			wordCount(wcount,temp,1);
		}
	}
	else if(temp->same_level){
		wordCount(wcount,temp,1);
	}
}

void writeWcFifo(wc wcount){

	int writefd;
	int digits=digs(getpid());
	char *wrName=godFather(getpid(),digits,0);
	if((writefd = open(wrName, O_WRONLY | O_APPEND) ) < 0){
		perror("worker: can't open write fifo");
		exit(1);
	}

	int worddigs=digs(wcount.word_freq);
	int chardigs=digs(wcount.char_freq);
	int linedigs=digs(wcount.line_freq);

	int total_length=worddigs+chardigs+linedigs+3;
	char finalString[total_length];
	sprintf(finalString,"%d\n%d\n%d",wcount.word_freq,wcount.char_freq,wcount.line_freq);
	///////////////////////////////////////////////
	if(write(writefd,finalString,total_length) != total_length){
		perror("worker: can't write fifo");
		exit(1);
	}

	int loglength=worddigs+chardigs+linedigs+33;
	char logstr[loglength];
	sprintf(logstr,"wc: words: %d-characters: %d-lines: %d\n",wcount.word_freq,wcount.char_freq,wcount.line_freq);
	writeLog((long)getpid(),"wc",logstr,"wc",0);

	close(writefd);
	free(wrName);
}

wc wc_init(int lines,int spaces){

	wc temp;
	temp.word_freq=0;
	temp.char_freq=spaces;
	temp.line_freq=lines;
	return temp;
}