#include<stdio.h>
#include<string.h>
#include<stdlib.h>

#define MAX_PAIRS 128
#define MAX_RECORD_SIZE 2*4096
#define MAX_ROWS 1024
#define MAX_COLS 128

typedef struct _file_entries{
	unsigned int rows;
	unsigned int cols;
	char *contents[MAX_ROWS][MAX_COLS];
}Relation;

typedef struct _similarCols{
	unsigned int colPairs[MAX_PAIRS][2];
	unsigned int length;
}similarColList;

int readFiles(Relation *file1,char *argv1,Relation *file2,char *argv2,similarColList *identicalCols);
void doCartesianProduct(Relation *l,Relation *r);
//void doNaturalJoin(Relation l,Relation r,similarColList list);
//void doLeftOuterJoin(Relation l,Relation r,similarColList list);
void printOptions(void);
int readOption(void);
void deAllocateMatrix(Relation *file1, Relation *file2);
void trim(char str[],int len);

int main(int argc,char *argv[]){
	int option;

	if(argc != 3){
		printf("USAGE :: %s <FILE 1> <FILE 2>\n",argv[0]);
		return -1;
	}

	Relation file1,file2;
	similarColList identicalCols;

	if(readFiles(&file1,argv[1],&file2,argv[2],&identicalCols)<0){
		printf("ERROR : There is issue to read file\n");
		return -1;
	}

	while(1){
		printf("Please Enter following options to execute the corresponding operation:\n");
		printOptions();
		option = readOption();
		switch(option){
		
		case 0:
			printf("Thank You for using this program.\n\t\t\t!!!! HAVE A WONDERFUL DAY !!!!\n");
			deAllocateMatrix(&file1,&file2);
			return -1;
		case 1:
			doCartesianProduct(&file1,&file2);
			break;
		case 2:
			printf("Sorry ! Natural Join is not running well due toremain less time to debug.\n");
//			doNaturalJoin(file1,file2,identicalCols);
			break;
		case 3:
			printf("Sorry ! Left Outer Join is not running well dueremain to less time to debug.\n");
//			doLeftOuterJoin(file1,file2,identicalCols);
			break;
		default:
			printf("Sorry ! Your input is incorrect.Please re-enter it\n");
			printOptions();
		}
	}
}

void trim(char str[],int length){
	if(length<=0){
		printf("string length is not valid\n");
		return;
	}

	int i = 0;
	int first_valid,last_valid;

	while(str[i] == ' ' || str[i] == '\t') i++;
	first_valid = i;
	i = length-1;
	while(str[i] == ' ' || str[i] == '\t') i--;
	last_valid = i;

	i = first_valid;
	int j = 0;
	while(i<=last_valid){
		str[j++] = str[i++];
	}
	str[j]='\0';
	return;
}

//Dispay options to user
void printOptions(void){
	printf("\tCartesian Product : 1\n");
	printf("\tNatural Join      : 2\n");
	printf("\tLeft Outer Join   : 3\n");
	printf("\tTo Quit           : 0\n");
}

//Waiting on User Input

int readOption(){
	int i;
	scanf("%d",&i);
	return i;
}

int readFiles(Relation *file1,char *argv1,Relation *file2,char *argv2,similarColList *identicalCols){
	char *header[2][MAX_COLS];
	char input[MAX_RECORD_SIZE];
	FILE *ptr1 = fopen(argv1,"r");
	FILE *ptr2 = fopen(argv2,"r");
	int i = 0, j = 0;
	char *token_ptr;
	char token[128],*ret;

	ret = fgets(input,MAX_RECORD_SIZE,ptr1);

	if(ret != NULL){
		input[strlen(input)-1] = '\0'; 
		j = 0;
		token_ptr = strtok(input,"|");
		while(token_ptr != NULL){
			strcpy(token,token_ptr);
			trim(token,strlen(token));	
			header[0][j]  = (char *) malloc (strlen(token)+1);
			strcpy(header[0][j],token);
			j++;
			token_ptr = strtok(NULL,"|");
		}
	}else{
		printf("No Content in the 1st file\n");
		return -1;
	}

	while(fgets(input,MAX_RECORD_SIZE,ptr1) != NULL){
		input[strlen(input)-1] = '\0'; 
		j = 0;
		token_ptr = strtok(input,"|");
		while(token_ptr != NULL){
			strcpy(token,token_ptr);
			trim(token,strlen(token));	
			file1->contents[i][j]  = (char *) malloc (strlen(token)+1);
			strcpy(file1->contents[i][j],token);
			j++;
			token_ptr = strtok(NULL,"|");
		}
		i++;
	}

	file1->rows = i;
	file1->cols = j;

	ret = fgets(input,MAX_RECORD_SIZE,ptr2);
	
	if(ret != NULL){
		input[strlen(input)-1] = '\0'; 
		j = 0;
		token_ptr = strtok(input,"|");
		while(token_ptr != NULL){
			strcpy(token,token_ptr);
			trim(token,strlen(token));	
			header[1][j]  = (char *) malloc (strlen(token)+1);
			strcpy(header[1][j],token);
			j++;
			token_ptr = strtok(NULL,"|");
		}
	}else{
		printf("No Content in the 2nd file\n");
		return -1;
	}

	i=0;
	j=0;

	while(fgets(input,MAX_RECORD_SIZE,ptr2) != NULL){
		input[strlen(input)-1] = '\0'; 
		j = 0;
		token_ptr = strtok(input,"|");
		while(token_ptr != NULL){
			strcpy(token,token_ptr);
			trim(token,strlen(token));	
			file2->contents[i][j]  = (char *) malloc (strlen(token)+1);
			strcpy(file2->contents[i][j],token);
			j++;
			token_ptr = strtok(NULL,"|");
		}
		i++;
	}

	file2->rows = i;
	file2->cols = j;

	identicalCols->length = 0;

	for(i=0;i<file1->cols;i++){
		for(j=0;j<file2->cols;j++){
			if(strcmp(header[0][i],header[1][j]) == 0){
				identicalCols->colPairs[identicalCols->length][0] = i;
				identicalCols->colPairs[identicalCols->length++][1] = j;
			}
		}
	}


	for(i=0;i<file1->cols;i++){
		if(header[0][i]){
		    free(header[0][i]);
		}
	}
	
	for(i=0;i<file2->cols;i++){
		if(header[1][i]){
		    free(header[1][i]);
		}
	}

	return 1;
}

void deAllocateMatrix(Relation *file1,Relation *file2){
	int i,j;

	for(i=0;i<file1->rows;i++){
		for(j=0;j<file1->cols;j++){
			if(file1->contents[i][j]){
		    		free(file1->contents[i][j]);
			}
		}
	}
	
	for(i=0;i<file2->rows;i++){
		for(j=0;j<file2->cols;j++){
			if(file2->contents[i][j]){
		    		free(file2->contents[i][j]);
			}
		}
	}
}

//Function for Cartesian Product

void doCartesianProduct(Relation *f1, Relation *f2){

	int row_l,col_l;
	int row_r,col_r;

	for(row_l=0;row_l<f1->rows;row_l++){

		for(row_r=0;row_r<f2->rows;row_r++){
			for(col_l = 0;col_l<f1->cols;col_l++){
				printf("%s",f1->contents[row_l][col_l]);
				printf(" | ");
			}

			for(col_r = 0;col_r<f2->cols;col_r++){
				printf("%s",f2->contents[row_r][col_r]);

				if(col_r<f2->cols-1)
					printf(" | ");
				else
					printf("\n");
			}
		}
	}
}

//Function for Natural Join
/*
void doNaturalJoin(Relation f1, Relation f2, similarColList identicalCols){

	int row_l,col_l;
	int row_r,col_r;

	unsigned int doInclude=0;
	unsigned int doNotPrint=0;

	int similarColIndex = 0;
	char str[MAX_RECORD_SIZE];
	char *firstRecord[MAX_PAIRS];

	if(identicalCols.length == 0){
		printf("No Columns are similar so , this is null set\n");
		return;
	}

	for(row_l=0;row_l<f1.rows;row_l++){

		for(row_r=0;row_r<f2.rows;row_r++){
			similarColIndex = 0;
			str[0] = '\0';
			doNotPrint = 0;

			for(col_l = 0;col_l<f1.col;col_l++){
				if(similarColIndex < identicalCols.length && identicalCols[similarColIndex][0] == col_l){
					firstRecord[similarColIndex] = f1.contents[row_l][col_l];
					similarColIndex++;
				}
				sprintf(str,"%s",f1.contents[row_l][col_l]);
				sprintf(str," | ");
			}
				
			similarColIndex = 0;
			for(col_r = 0;col_r<f2.col;col_r++){
				doInclude = 0;
				if(similarColIndex < identicalCols.length && identicalCols[similarColIndex][1] == col_r){

					if(strcmp(firstRecord[similarColIndex],f2.contents[row_r][col_r]) == 0){
						similarColIndex++;
						doInclude = 1;
					}else{
						doNotPrint = 1;
						break;
					}
				}
				if(!doInclude){
					sprintf(str,"%s",f2.contents[row_r][col_r]);
					if(col_r<f2.col-1)
						sprintf(str," | ");
					else
						sprintf(str,"\n");
				}
			}

			if(!doNotPrint){
				int len = strlen(str);
				int i = len-1;
				if(len > 0 && str[i] != '\n'){
					while((i >= 0) && (str[i] == ' ' || str[i] == '|'))
						i--;
					str[i+1] = '\n';
					str[i+2] = '\0';
				}
				printf("%s",str);
			}
		}
	}
}
*/
