#include "jobExecutor.h"

long proConn(int *SOELine,char **sender,int index,int from){

	int i;
	signal(SIGUSR1,handler);
	signal(SIGUSR2,handler);
	resume=2;
	endProc=0;
	pid_t child=fork();

	switch( child ) {
 		case -1:{
			perror( "fork" );
			exit( 1 );
		}
		case 0:{
			/*child process*/
			/////////////////start workers/////////////////
			if(resume!=1)
				resume=0;
			while(!resume);
			int digits=digs(getpid());
			char *rdName=godFather(getpid(),digits,1);
			int readfd;
			if( (readfd = open(rdName, O_RDONLY) ) < 0){
				perror("worker: can't open read fifo");
			}

			char **path_array=malloc(index*sizeof(char *));
			ssize_t count;
			for(i=0;i<index;i++){
				path_array[i]=malloc((SOELine[from]+1)*sizeof(char ));
				count=read( readfd, path_array[i], SOELine[from++] );
				if ( count <= 0 ) {
					perror( "read" );
					exit(1);
				}
				path_array[i][count] = '\0';
			}
			close(readfd);
			///////count files inside every directory//////
			int fileCounter=0;
			for(i=0;i<index;i++){
				chdir(getenv("HOME"));
				DIR* dir1;
				struct dirent *sd1;
				dir1=opendir(path_array[i]);
				if(!dir1){
					perror("opendir error");
					exit(1);
				}
				chdir(path_array[i]);
				while((sd1=readdir(dir1))){
					if(strcmp(sd1->d_name,".") && strcmp(sd1->d_name,"..")){
						fileCounter++;
					}
				}
				closedir(dir1);
			}
			//open files-create one trie for every worker//
			trie *root=init();
			int lineCounter=0,spaces=0;
			for(i=0;i<index;i++){
				int j;
				chdir(getenv("HOME"));
				DIR* dir;
				struct dirent *sd;
				dir=opendir(path_array[i]);
				if(!dir){
					perror("opendir error");
					exit(1);
				}
				chdir(path_array[i]);				
				while((sd=readdir(dir))){
					if(strcmp(sd->d_name,".") && strcmp(sd->d_name,"..")){
						FILE *fp = fopen(sd->d_name,"r");
						char *newPath=newPathName(path_array[i],sd->d_name);
						if(!fp){
							perror("error opening the file");
							exit(1);
						}
						char **map_arr=NULL;
						int *soeline=NULL;
						int nol=mapping(sd->d_name,&map_arr,&soeline);
						lineCounter+=nol;
						////////////////inverted index////////////////
						for(j=0;j<nol;j++){
							int chcounter=0;
							char *content=malloc((soeline[j]+1)*sizeof(char));
							char *frcontent=content;
							strcpy(content,map_arr[j]);
							//count spaces for wc
							while(content[chcounter]!='\0'){
								if(content[chcounter]==' '){
									spaces++;
								}
								chcounter++;
							}
							chcounter=0;
							do{
								while(content[chcounter]!=' ' && content[chcounter]!='\0'){
									chcounter++;
								}
								char *word=malloc((chcounter+1)*sizeof(char));
								strncpy(word,content,chcounter);
								word[chcounter]='\0';
								content+=chcounter;
								if(content[0]==' '){
									content++;
								}
								insertion(&root,j,word,newPath);
								free(word);
								chcounter=0;														
							}while(content[0]!='\0');
							free(frcontent);
						}
						//////////////////////////////////////////////
						for(j=0;j<nol;j++)
							free(map_arr[j]);
						free(map_arr);
						free(soeline);
						free(newPath);
						fclose(fp);			
					}
				}
				closedir(dir);
			}
			////////////////function to call///////////////
			do{
				raise(SIGSTOP);
				if(!endProc){
					if( (readfd = open(rdName, O_RDONLY) ) < 0){
						perror("worker: can't open read fifo");
					}
					char buf[MAXBUFF];
					if((count=read(readfd,buf,MAXBUFF))<0){
						perror( "read" );
						exit(1);
					}
					close(readfd);
			///////////////////////////////////////////////
					char **command=NULL;
					int size=str_split(buf,&command);
					caller(root,command,size,fileCounter,lineCounter,spaces);
			///////////////////////////////////////////////
					for(i=0;i<size;i++)
						free(command[i]);
					free(command);
				}
			}while(!endProc);
			///////////////////////////////////////////////
			destroyNodes(&root,0);
			free(root);
			if(unlink(rdName)<0){
				perror("Worker can't unlink\n");
			}
			free(rdName);
			for(i=0;i<index;i++)
				free(path_array[i]);
			free(path_array);
			exit(0);
		}
		default:{
			/*parent process*/
			/////////////find name for each fifo////////////
			int digits=digs(child);
			char *wrName=godFather(child,digits,1);
			char *rdName=godFather(child,digits,0);
			///////////////////////////////////////////////
			int writefd;
			if(mkfifo(wrName, PERMS) < 0){
				perror("can't create child's read fifo");
			}
			if(mkfifo(rdName, PERMS) < 0){
				perror("can't create child's write fifo");
			}
			kill((long)child,SIGUSR1);

			if((writefd = open(wrName, O_WRONLY) ) < 0){
				perror("jobExecutor: can't open write fifo");
			}
			for(i=0;i<index;i++){
				size_t length = strlen( sender[i] );
				if(write( writefd, sender[i], length )!=length){
					perror("jobExecutor: can't write fifo");
				}
			}
			close(writefd);
			int status;
			waitpid(child,&status,WUNTRACED);
			if(WTERMSIG(status)==11){
				printf("Process corrupts-sigseg\n");
				exit(1);
			}
			free(wrName);
			free(rdName);
			return child;
		}
	}
}

void handler(int sig){

	if(sig==SIGUSR1){
		resume=1;
	}
	else{
		endProc=1;
	}
}

int execQueries(long *pids,int numOfWorkers,char **input,int queries,double deadline,
	char **map_array,int *SOELine,int numOfDirectories){

	time_t start=time(NULL);
	int timeOut=0;
	int i,j,status;
	int min=0,max=0;
	char *maxPath=NULL;
	char *minPath=NULL;
	wc wCount=wc_init(0,0);
	///////////////appropriate form////////////////
	int total_length=0;
	for(j=0;j<queries;j++)
		total_length+=(strlen(input[j])+1);
	char finalString[total_length];
	int offset=0;
	int length;
	for(j=0;j<queries;j++){
		length=strlen(input[j]);
		strcpy(finalString+offset,input[j]);
		if(j!=(queries-1))
			finalString[length+offset]='\n';
		offset+=(length+1);
	}
	///////////////////////////////////////////////
	for(i=0;i<numOfWorkers;i++){
		////////check if process is still alive////////
		pid_t result=waitpid(pids[i],&status,WNOHANG);
		if(result==-1){
			perror("on checking if child is alive");
			exit(1);
		}
		else if(result!=0){
		//////////////or create a new one//////////////
			int digits=digs(pids[i]);
			char *wrName=godFather(pids[i],digits,1);
			char *rdName=godFather(pids[i],digits,0);
			if(unlink(rdName)<0){
				perror("Worker can't unlink\n");
			}
			free(rdName);
			if(unlink(wrName)<0){
				perror("Worker can't unlink\n");
			}
			free(wrName);
		///////////////////////////////////////////////
			int x=(numOfDirectories / numOfWorkers);
			int y=(numOfDirectories % numOfWorkers);
			int index=0;
			if(i<y){
				char **sender=malloc((x+1)*sizeof(char *));
				int from=(x+1)*i;
				for(j=from; j< from+(x+1); j++){
					sender[index]=malloc((SOELine[j]+1)*sizeof(char ));
					strcpy(sender[index],map_array[j]);
					index++;
				}
				pids[i]=proConn(SOELine,sender,index,from);

				for(j=0;j<index;j++)
					free(sender[j]);
				free(sender);
			}
			else{
				char **sender=malloc(x*sizeof(char *));
				int from=((x+1)*i - (i-y));
				for(j= from; j< from+x; j++){
					sender[index]=malloc((SOELine[j]+1)*sizeof(char ));
					strcpy(sender[index],map_array[j]);
					index++;
				}
				pids[i]=proConn(SOELine,sender,index,from);

				for(j=0;j<index;j++)
					free(sender[j]);
				free(sender);
			}
		}
		///////////////////////////////////////////////
		kill(pids[i],SIGCONT);
		/////////////find each fifo's name/////////////
		int digits=digs(pids[i]);
		char *wrName=godFather(pids[i],digits,1);
		char *rdName=godFather(pids[i],digits,0);
		///////////////////////////////////////////////
		int writefd,readfd;
		if((writefd = open(wrName, O_WRONLY) ) < 0){
			perror("jobExecutor: can't open write fifo");
			exit(1);
		}

		if(write(writefd,finalString,total_length) != total_length){
			perror("jobExecutor: can't write fifo");
			exit(1);
		}
		close(writefd);
		///////////////wait for results////////////////
		if((readfd = open(rdName, O_RDONLY) ) < 0){
			perror("jobExecutor: can't open read fifo");
			exit(1);
		}
		int count;
		char buf[MAXBUFF];
		while(1){
			memset(buf,0,MAXBUFF);
			if((count=read(readfd,buf,MAXBUFF))<0){
				perror( "read" );
				exit(1);
			}
			if(count==0)
				break;
			/////////////////////output/////////////////////
			if(strcmp(input[0],"/search")==0){
				if(time(NULL) < start+deadline){
					for(j=0;j<count;j++){
						if(buf[j]!='\0')
							printf("%c",buf[j]);
					}
				}
				else if((time(NULL) >= start+deadline) && timeOut==0){
					timeOut=1;
					close(readfd);
					free(wrName);
					free(rdName);
					return i;
				}
			}
			else if(strcmp(input[0],"/maxcount")==0){
				char **res=NULL;
				int k=str_split(buf,&res);
				if(max==0 && atoi(res[1])>max){
					max=atoi(res[1]);
					maxPath=malloc((strlen(res[0])+1)*sizeof(char ));
					strcpy(maxPath,res[0]);
				}
				else{
					if(atoi(res[1])>max){
						free(maxPath);
						max=atoi(res[1]);
						maxPath=malloc((strlen(res[0])+1)*sizeof(char ));
						strcpy(maxPath,res[0]);
					}
					else if(atoi(res[1])==max){
						int temp=alphaGreater(maxPath,res[0]);
						if(temp==1){
							free(maxPath);
							maxPath=malloc((strlen(res[0])+1)*sizeof(char ));
							strcpy(maxPath,res[0]);
						}
					}
				}
				for(j=0;j<k;j++)
					free(res[j]);
				free(res);
			}
			else if(strcmp(input[0],"/mincount")==0){
				char **res=NULL;
				int k=str_split(buf,&res);
				if(min==0 && atoi(res[1])>min){
					min=atoi(res[1]);
					minPath=malloc((strlen(res[0])+1)*sizeof(char ));
					strcpy(minPath,res[0]);
				}
				else{
					if(atoi(res[1])<min){
						free(minPath);
						min=atoi(res[1]);
						minPath=malloc((strlen(res[0])+1)*sizeof(char ));
						strcpy(minPath,res[0]);
					}
					else if(atoi(res[1])==min){
						int temp=alphaGreater(minPath,res[0]);
						if(temp==1){
							free(minPath);
							minPath=malloc((strlen(res[0])+1)*sizeof(char ));
							strcpy(minPath,res[0]);
						}
					}
				}
				for(j=0;j<k;j++)
					free(res[j]);
				free(res);
			}
			else{
				char **res;
				int k=str_split(buf,&res);
				wCount.word_freq+=atoi(res[0]);
				wCount.char_freq+=atoi(res[1]);
				wCount.line_freq+=atoi(res[2]);
				for(j=0;j<k;j++)
					free(res[j]);
				free(res);
			}
		}
		waitpid(pids[i],&status,WUNTRACED);
		if(i==(numOfWorkers-1) && max>0){
			printf("%s : %d\n",maxPath,max);
		}
		if(i==(numOfWorkers-1) && min>0){
			printf("%s : %d\n",minPath,min);
		}
		if(i==(numOfWorkers-1) && (wCount.word_freq)>0){
			printf("Total number of words: %d\n",wCount.word_freq);
			printf("Total number of characters: %d\n",wCount.char_freq);
			printf("Total number of lines: %d\n",wCount.line_freq);
		}
		///////////////////////////////////////////////
		close(readfd);
		free(wrName);
		free(rdName);
	}
	if(maxPath)
		free(maxPath);
	if(minPath)
		free(minPath);
	return 0;
}

char *godFather(long id,int digits,int flag){

	//string needs 12+digits+1+4 bytes=>digits+17 bytes
	char *name=malloc((digits+17)*sizeof(char));
	if(flag==0)
		sprintf(name,"/tmp/pipeWR_%ld.txt",id);
	else
		sprintf(name,"/tmp/pipeRD_%ld.txt",id);

	return name;
}

int digs(long id){

	long x=id;
	int digits=0;
	while(x>0){
		x/=10;
		digits++;
	}
	return digits;
}

int mapping(char *filename,char ***map_arr,int **soeline){

	FILE *f = fopen(filename,"r");
	int i=0,ch=0,index=0;
	int numOfLines=countLines(filename);
	int maxStringLength=mSL(filename,numOfLines);
	char temp[maxStringLength];
	*map_arr=malloc(numOfLines*sizeof(char *));
	*soeline=malloc(numOfLines*sizeof(int));
	while(i<numOfLines){
		ch=fgetc(f);
		if(ch=='\n'){
			temp[index]='\0';
			((*soeline)[i])=strlen(temp);
			((*map_arr)[i])=malloc(((*soeline)[i])*sizeof(char )+1);
			strcpy(((*map_arr)[i]),temp);
			index=0;
			i++;
		}
		else if(index==0 && (ch==' ' || ch=='\t')){
			continue;
		}
		else if(ch=='\t'){
			temp[index++]=' ';
		}
		else{
			temp[index++]=ch;
		}
	}
	fclose(f);
	return numOfLines;
}

char *newPathName(char *path,char *addition){

	char *newpath=malloc( ( (int)strlen(path) + (int)strlen(addition) +2 )*sizeof(char) );
	sprintf(newpath,"%s/%s",path,addition);

	return newpath;
}

int str_split(char *str,char ***command){

	int i=0,wordcounter=1;
	while(str[i]!='\0'){
		if(str[i]=='\n')
			wordcounter++;
		i++;
	}

	int *soeline=malloc(wordcounter*sizeof(int ));
	for(i=0;i<wordcounter;i++)
		soeline[i]=0;
	i=0;
	int counter=0;
	while(str[i]!='\0'){
		if(str[i]!='\n'){
			soeline[counter]++;
		}
		else{
			counter++;
		}
		i++;
	}

	(*command)=malloc(wordcounter*sizeof(char *));
	for(i=0;i<wordcounter;i++){
		(*command)[i]=malloc((soeline[i]+1)*sizeof(char ));
		memset((*command)[i],0,(soeline[i]+1));
	}

	int offset=0;
	for(i=0;i<wordcounter;i++){
		strncpy((*command)[i],str+offset,soeline[i]);
		offset+=(soeline[i]+1);
	}

	free(soeline);
	return wordcounter;
}

int alphaGreater(char *str1,char *str2){

	int i=0,ch=0;
	while(str1[ch] && str2[ch]){
		ch++;
	}

	while(i<ch){
		if(str1[i]>str2[i])
			return 1;
		else if(str1[i]<str2[i])
			return 0;
		else
			i++;
	}
	if(str1[ch]!='\0')
		return 1;
	else
		return 0;
}

void caller(trie *root,char **command,int size,int fileCounter,int lineCounter,int spaces){

	if(strcmp(command[0],"/search")==0){
		search(root,command,size);
	}
	else if(strcmp(command[0],"/maxcount")==0){
		maxmin_Count(root,command[1],fileCounter,0);
	}
	else if(strcmp(command[0],"/mincount")==0){
		maxmin_Count(root,command[1],fileCounter,1);
	}
	else if(strcmp(command[0],"/wc")==0){
		wc wcount=wc_init(lineCounter,spaces);
		wordCount(&wcount,root,0);
		writeWcFifo(wcount);
	}
}

void writeLog(long childpid,char *queryType,char *query,char *path,int flag){

	char name[1024];
	sprintf(name,"%s/log/Worker_<%ld>.log",cwd,childpid);

	FILE *fp = fopen(name, "a");
	if(!fp){
		perror("creating/writing to the log file");
		exit(1);
	}

	if(!flag){
		time_t timer;
		char buffer[26];
		struct tm* tm_info;
		time(&timer);
		tm_info=localtime(&timer);
		strftime(buffer,26,"%Y-%m-%d %H:%M:%S",tm_info);

		fseek(fp,0,SEEK_END);
		int size=ftell(fp);
		if(!strcmp(queryType,"wc")){
			if(size==0)
				fprintf(fp,"%s: %s",buffer,query);
			else
				fprintf(fp,"\n%s: %s",buffer,query);

		}
		else{
			if(size==0)
				fprintf(fp,"%s: %s: %s: %s",buffer,queryType,query,path);
			else
				fprintf(fp,"\n%s: %s: %s: %s",buffer,queryType,query,path);
		}
	}
	else{
		fprintf(fp,":%s",path);
	}
	fclose(fp);
}