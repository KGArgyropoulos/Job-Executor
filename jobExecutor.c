#include "jobExecutor.h"

int main( int argc, char *argv[]){

	//we need our program's absolute path for later use
	getcwd(cwd, sizeof(cwd));
	//////////////////open file//////////////////
	FILE *f;
	int numOfWorkers;
	char *filename;
	if(argc == 3){
		if(strcmp(argv[1],"-d")==0){
			f = fopen(argv[2], "r");
			filename=argv[2];
			numOfWorkers=10;//default value if -w option is not given
		}
		else{
			printf("Error in passing the options\n");
			return -1;
		}
	}
	else if(argc == 5){
		if(strcmp(argv[1],"-d")==0){
			f = fopen(argv[2], "r");
			filename=argv[2];
			if(strcmp(argv[3],"-w")==0){
				numOfWorkers=atoi(argv[4]);
			}
			else{
				printf("Error in passing the options\n");
				return -1;
			}
		}
		else if(strcmp(argv[1],"-w")==0){
			numOfWorkers=atoi(argv[2]);
			if(strcmp(argv[3],"-d")==0){
				f = fopen(argv[4],"r");
				filename=argv[4];
			}
			else{
				printf("Error in passing the options\n");
				return -1;
			}
		}
		else{
			printf("Error in passing the options\n");
			return -1;
		}
	}
	else{
		printf("Error in passing the options\n");
		return -1;
	}
	if(numOfWorkers<1){
		printf("Too few workers\n");
		return -1;
	}

	///////////check if the file is found///////////
	if (f == NULL){
		printf("Failed to open the file you typed\n");
		return -2;
	}

	///////////////////map array///////////////////
	char **map_array=NULL;
	int *SOELine=NULL;
	int numOfDirectories=mapping(filename,&map_array,&SOELine);
	fclose(f);//close the file,now that we have everything in memory

	/////////////////start workers/////////////////
	/*
	Algorithm for spliting directories to workers
	directories |workers
				|_______
				|
			y	|x
				|

	y workers will take x+1 directories and (workers-y) workers will take x directories
	[y*(x+1)+(workers-y)*x]
	*/
	int x=(numOfDirectories / numOfWorkers);
	int y=(numOfDirectories % numOfWorkers);
	int i,j=0,index=0;
	long *pids=malloc(numOfWorkers*sizeof(long ));

	for(i=0;i<y;i++){
		char **sender=malloc((x+1)*sizeof(char *));
		index=0;
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
	for(i=y;i<numOfWorkers;i++){
		char **sender=malloc(x*sizeof(char *));
		index=0;
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

	//////////////command to be given//////////////
	char **input;
	int ch=0,time=1;
	do{
		if(time>1){
			for(i=0;i<11;i++){
				free(input[i]);
			}
			free(input);
		}

		printf("Please,type which of our services you would like to access\n");
		input=malloc(11*sizeof(char *));
		for(i=0;i<11;i++)
			input[i]=NULL;

		ch=0;
		index=0;
		while(ch!='\n' && index<11){
			scanf("%ms",&input[index++]);
			ch=getchar();
		}
		double deadline;

		if(index==11){
			if(strcmp(input[10],"-d")==0){
				scanf("%lf",&deadline);
			}
			else{
				printf("Search up to 10 words.Rest of them will be ignored\n");
				while(ch!='\n'){
					ch=getchar();
					if(ch=='-'){
						ch=getchar();
						char *num;
						if(ch=='d'){
							ch=getchar();
							scanf("%ms",&num);
							deadline=atof(num);
						}
					}
				}
			}
		}
		int control;
		if(strcmp(input[0],"/search")==0){
			for(i=0;i<index;i++){
				if(strcmp(input[i],"-d")==0){
					if(i!=10){
						deadline=atof(input[++i]);
						index-=2;
					}
					else{
						index-=1;
					}
					break;
				}
			}
			control=execQueries(pids,numOfWorkers,input,index,deadline,map_array,SOELine,numOfDirectories);
			if(control)
				printf("%d out of %d workers answered in time\n",control,numOfWorkers);
		}
		else if(strcmp(input[0],"/maxcount")==0){
			if(input[1]!=NULL && input[2]==NULL){
				control=execQueries(pids,numOfWorkers,input,index,0.0,map_array,SOELine,numOfDirectories);
			}
			else{
				printf("Error typing arguments for maxcount command\n");
			}
		}
		else if(strcmp(input[0],"/mincount")==0){
			if(input[1]!=NULL && input[2]==NULL){
				control=execQueries(pids,numOfWorkers,input,index,0.0,map_array,SOELine,numOfDirectories);
			}
			else{
				printf("Error typing arguments for mincount command\n");
			}
		}
		else if(strcmp(input[0],"/wc")==0){
			if(input[1]==NULL){
				control=execQueries(pids,numOfWorkers,input,index,0.0,map_array,SOELine,numOfDirectories);
			}
			else{
				printf("Error typing arguments for wc command\n");
			}
		}
		else if(strcmp(input[0],"/exit")!=0){
			printf("Error typing the command\n");
		}

		time++;
	}while(strcmp(input[0],"/exit")!=0);

	for(i=0;i<numOfWorkers;i++){
		kill(pids[i],SIGUSR2);
		kill(pids[i],SIGCONT);
		wait(NULL);
	}

	for(i=0;i<11;i++){
		free(input[i]);
	}
	free(input);
	///////////////////////////////////////////////
	int digits;
	for(i=0;i<numOfWorkers;i++){
		digits=digs(pids[i]);
		char *tempName=godFather(pids[i],digits,0);
		if(unlink(tempName)<0){
			perror("Worker can't unlink\n");
		}
		free(tempName);
	}
	free(pids);
	for(i=0;i<numOfDirectories;i++)
		free(map_array[i]);
	free(map_array);
	free(SOELine);

	return 0;
}

int countLines(char *filename){

	FILE *fp = fopen(filename,"r");
	if(!fp)
		printf("Failed to open file, named: %s\n",filename);
	int ch=0,lines=0;
	while(!feof(fp)){
		ch = fgetc(fp);
		if(ch == '\n'){
			lines++;
		}
	}
	
	fclose(fp);
	return lines;
}

int mSL(char *filename,int numOfLines){

	FILE *fp = fopen(filename,"r");	
	int i,ch=0;
	int maxLengthOfLine=0;
	int lengthOfEachLine=0;
	for(i=0;i<numOfLines;i++){
		do{
			ch=fgetc(fp);
			lengthOfEachLine++;
		}while(ch!='\n');

		if(lengthOfEachLine>maxLengthOfLine){
			maxLengthOfLine=lengthOfEachLine;
		}
		lengthOfEachLine=0;
	}

	fclose(fp);
	return maxLengthOfLine;
}