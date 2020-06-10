#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <sqlite3.h>

#define nrQuestions 2
#define nrRounds 1
/* portul folosit */
#define PORT 2909
#define MAX_STR 15
int nrPlayers=0;
int waitingPlayers=0;

/* codul de eroare returnat de anumite apeluri */
extern int errno;

static void *treatLogin(void *); /* functia executata de fiecare thread ce realizeaza comunicarea cu clientii */
//void raspunde(void *);
typedef struct {
  char username[20] ; 
  int cl;
  char rank[MAX_STR];
  int nrgames;
  int victories;
}player;

typedef struct {
	pthread_t idThread; //id-ul thread-ului
	int thCount; //nr de conexiuni servite
}Thread;

Thread *threadsPool; //un array de structuri Thread pentru login
player playersPool[4];
int sd; //descriptorul de socket de ascultare
int nthreads=3;//numarul de threaduri
pthread_mutex_t mlock=PTHREAD_MUTEX_INITIALIZER;              // variabila mutex ce va fi partajata de threaduri
pthread_mutex_t glock=PTHREAD_MUTEX_INITIALIZER;

void raspunde(int cl,int idThread);
void addClient_WaitingList(char username[20],int cl,char rank[15],int nrGames,int nrVictories,int idThread);
void info(sqlite3* dbUsers,char rank[15],int *nrGames,int *nrVictories,char username[20]);
void send_question(sqlite3_stmt *getQ,player players[4],int idThread);
//void wait_Answers(player players[4],sqlite3_stmt *getQ,int score[4],int idThread);
void start_Time(player players[4],int idThread);

int main (int argc, char *argv[])
{ 
  struct sockaddr_in server;	// structura folosita de server  	
  void threadCreate(int);
  
   if(argc<2)
   {
        fprintf(stderr,"Eroare: Primul argument este numarul de fire de executie...");
	exit(1);
   }
 
    threadsPool = calloc(sizeof(Thread),nthreads);

   /* crearea unui socket */
  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("[server]Eroare la socket().\n");
      return errno;
    }
  /* utilizarea optiunii SO_REUSEADDR */
  int on=1;
  setsockopt(sd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
  
  /* pregatirea structurilor de date */
  bzero (&server, sizeof (server));

  /* umplem structura folosita de server */
  /* stabilirea familiei de socket-uri */
    server.sin_family = AF_INET;	
  /* acceptam orice adresa */
    server.sin_addr.s_addr = htonl (INADDR_ANY);
  /* utilizam un port utilizator */
    server.sin_port = htons (PORT);
  
  /* atasam socketul */
  if (bind (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1)
    {
      perror ("[server]Eroare la bind().\n");
      return errno;
    }

  /* punem serverul sa asculte daca vin clienti sa se conecteze */
  if (listen (sd, 2) == -1)
    {
      perror ("[server]Eroare la listen().\n");
      return errno;
    }

   printf("Nr threaduri %d \n", nthreads); fflush(stdout);
   int i;
   for(i=0; i<nthreads;i++) loginThreadCreate(i);
   

	
  /* servim in mod concurent clientii...folosind thread-uri */
  for ( ; ; ) 
  {
	printf ("[server]Asteptam la portul %d...\n",PORT);
        pause();				
  }
};
				
void loginThreadCreate(int i)
{
	void *treatLogin(void *);
	
	pthread_create(&threadsPool[i].idThread,NULL,&treatLogin,(void*)i);
	return; /* threadul principal returneaza */
}

 

void *treatLogin(void * arg)
{		
		int client;
		        
		struct sockaddr_in from; 
 	        bzero (&from, sizeof (from));
 		printf ("[thread login]- %d - pornit...\n", (int) arg);fflush(stdout);

		for( ; ; )
		{
			int length = sizeof (from);
			pthread_mutex_lock(&mlock);
			printf("Thread %d trezit\n",(int)arg);
			if ( (client = accept (sd, (struct sockaddr *) &from, &length)) < 0)
       	 	{
        	 perror ("[thread]Eroare la accept().\n");          
        	}
			pthread_mutex_unlock(&mlock);
			threadsPool[(int)arg].thCount++;

			raspunde(client,(int)arg); //procesarea cererii
			/* am terminat cu acest client, inchidem conexiunea */
					
		}	
}


void game( player players[4],int idThread){
 	
 	char message[20]="A INCEPUT JOCUL\n";
 	for(int i=0;i<4;i++)
  		if(write(players[i].cl,message,sizeof(message))<=0){
     		   printf("[Thread %d]\n",idThread);
               perror ("Eroare la write() pentru anuntarea inceperii jocului !\n");
              }
    printf("Trimitem oponentii:\n");
    for(int i=0;i<4;i++)
    {   for(int k=0;k<4;k++)
    	{if(i!=k)
    	{if(write(players[i].cl,players[k].username,sizeof(players[k].username))<0){
     		   printf("[Thread %d]\n",idThread);
               perror ("Eroare la write() ->numele adversarilor !\n");
              }

        if(write(players[i].cl,players[k].rank,sizeof(players[k].rank))<0){
     		   printf("[Thread %d]\n",idThread);
               perror ("Eroare la write() ->numele adversarilor !\n");
              }
        }
 		}
 		   
    }


     char tables[2][10];
     strcpy(tables[0],"general");
     strcpy(tables[1],"science");
    //deschide baza de date cu intrebari
  	sqlite3* dbQues;
  	int score[4];for(int i=0;i<4;i++) score[i]=0;
  
  
   
    sqlite3_stmt *getQ;

    char query[30];
    if(sqlite3_open("questions.db",&dbQues)!=0){
        fprintf(stderr, "Can't open questions database: %s\n", sqlite3_errmsg(dbQues));
        sqlite3_close(dbQues);
       }

   
   for(int nrRound=0;nrRound<nrRounds;nrRound++){
    	sprintf(query,"select * from %s;",tables[nrRound]);
        int errorQ=sqlite3_prepare_v2(dbQues,query,-1,&getQ,0);
        		
     	if(errorQ!=SQLITE_OK)
   	    	{fprintf(stderr, "Can't get data %s\n", sqlite3_errmsg(dbQues));
        	sqlite3_close(dbQues);}

   		for(int indQ=0;indQ<nrQuestions;indQ++){
   			printf("Intrebarea %d\n",indQ);
    		errorQ=sqlite3_step(getQ);  
  
  
        	if(errorQ==SQLITE_ROW)
    	  	     send_question(getQ,players,idThread);

    	    start_Time(players,idThread);
    	    
    		int answer;
    	 	int corect;
    	 	for(int i=0;i<4;i++)
           	 {   if(read(players[i].cl,&answer,sizeof(answer))<0)
    	            perror("Eroare la citirea raspunsului\n");
             	    else 
             		printf("Raspuns: %d \n",answer);
             	int nr=sqlite3_column_int64(getQ,5);
             	
             	if(answer==sqlite3_column_int64(getQ,5))
             		corect=1;
             	else corect=0;
             	if(write(players[i].cl,&corect,sizeof(int))<0)
             		 perror("Eroare la write() corect/incorect\n");
             	score[i]+=corect;

             }
    	 }
    		 sqlite3_finalize(getQ);
    }
    

    sqlite3_close(dbQues);	
    printf("End of the game!\n");
    int order[4],aux;
    for(int i=0;i<4;i++) order[i]=i;

    for(int i=0;i<3;i++)
    	for(int j=i+1;j<4;j++)
    		if(score[i]<score[j]){
    			aux=score[j];
    			score[j]=score[i];
    			score[i]=aux;
    			aux=order[i];
    			order[i]=order[j];
    			order[j]=aux;
    		}

    		//TRIMITEREA CLASAMENTULUI
    for(int i=0;i<4;i++)
     for(int j=0;j<4;j++)
    {	if(write(players[i].cl,players[order[j]].username,sizeof(players[order[j]].username))<0)
    	perror("Thread %d: Eroare la trimiterea clasamentului\n");
    	if(write(players[i].cl,score+j,sizeof(int))<0)
    		perror("Eroare la write->loc\n");
    }

    sqlite3 *dbUsers;
     if(sqlite3_open("accounts.db",&dbUsers)!=0){

          fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(dbUsers));
          sqlite3_close(dbUsers);
            }

    
    int maxScore=score[0];
    char qset[100];
    int nr;
    char rang[15];
    for(int i=0;i<4;i++){

    	nr=players[order[i]].nrgames+1;
    	sprintf(qset,"update users set nrgames=%d where username like '%s';",nr,players[order[i]].username);
    	if(sqlite3_exec(dbUsers,qset,NULL,NULL,NULL)!=0)
             {
                fprintf(stderr, "Can't execute query on accounts database: %s\n", sqlite3_errmsg(dbUsers));
                sqlite3_close(dbUsers);
                   
             }
             strcpy(rang,players[order[i]].rank);
    	if(score[i]==maxScore){
    		nr=players[order[i]].victories+1;
    		sprintf(qset,"update users set nrvictories=%d where username like '%s';",nr,players[order[i]].username);
    		
    		if(sqlite3_exec(dbUsers,qset,NULL,NULL,NULL)!=0)
             {
                fprintf(stderr, "Can't execute query on accounts database: %s\n", sqlite3_errmsg(dbUsers));
                sqlite3_close(dbUsers);
                   
             }
    		if(players[order[i]].nrgames>100&& players[order[i]].victories>50/100*players[order[i]].nrgames)
    			{  strcpy(rang,"medium");
    				
    			}
    		
    		}
    	
    		if(write(players[order[i]].cl,rang,sizeof(rang))<0)
    					perror("ERoare la trimiterea rangului\n");

     }
     sqlite3_close(dbUsers);
    
    int request[4];
    int nrRequests=0;;
    for(int i=0;i<4;i++)
    	{if(read(players[i].cl,request+i,sizeof(int))<0)
    		perror("Eroare la read->loc new game\n");

    	nrRequests+=request[i];
    	}
    	printf("Requests for a new game:%d\n",nrRequests);
    if(nrRequests>0)
    {pthread_mutex_lock(&glock);
 		printf("Punem pe lista de asteptare jucatorii\n");
    	if(nrRequests+waitingPlayers>=4)
    	{
    		player newgame[4];
        for(int j=0;j<waitingPlayers;j++)
                  {
                    newgame[j].cl=playersPool[j].cl;
                    newgame[j].nrgames=playersPool[j].nrgames;
                    newgame[j].victories=playersPool[j].victories;
                    strcpy(newgame[j].username, playersPool[j].username);
                    strcpy(newgame[j].rank, playersPool[j].rank);
                     
                    }
        
        int nr=waitingPlayers;
        waitingPlayers=0;
        for(int j=0;j<4;j++)
        	if(request[j]==1&&nr<4)
         		{	
                    newgame[nr].cl=players[j].cl;
                    newgame[nr].nrgames=players[j].nrgames;
                    newgame[nr].victories=players[j].victories;
                    strcpy(newgame[nr].username, players[j].username);
                    strcpy(newgame[nr].rank, players[j].rank);
                    nr++;
                   }  
                    
        	else if(request[j]==1 && nr==4)
        		{strcpy(playersPool[waitingPlayers].username,players[j].username);
        		 strcpy(playersPool[waitingPlayers].rank,players[j].rank);
     			 playersPool[waitingPlayers].cl=players[j].cl;
     			 playersPool[waitingPlayers].nrgames=players[j].nrgames;
     			 playersPool[waitingPlayers].victories=players[j].victories;
     			 waitingPlayers++;}

     	pthread_mutex_unlock(&glock);
        game(newgame,idThread);
    	} else
    	{    for(int j=0;j<4;j++)
        	   if(request[j]==1)
        		{strcpy(playersPool[waitingPlayers].username,players[j].username);
        		 strcpy(playersPool[waitingPlayers].rank,players[j].rank);
     			 playersPool[waitingPlayers].cl=players[j].cl;
     			 playersPool[waitingPlayers].nrgames=players[j].nrgames;
     			 playersPool[waitingPlayers].victories=players[j].victories;
     			 waitingPlayers++;}
     			 pthread_mutex_unlock(&glock);

    	}
    	

    }

    }
   
     //	sprintf(query,"select * from trivia  LIMIT 1 OFFSET %d",i);
     	

    
  


void start_Time(player players[4],int idThread){
	int ok=1;
	for(int i=0;i<4;i++)
		if (write(players[i].cl,&ok,sizeof(int))<0)
			{printf("[Thread %d]\n",idThread);
			perror("Eroare la write() la start_Time\n");}
}

void send_question(sqlite3_stmt *getQ,player players[4],int idThread){
	char question[400]; question[0]='\0';
    int length;
    printf("\n\n");
	for(int i=0;i<5;i++)
    		{   strcpy(question,sqlite3_column_text(getQ,i));
    			question[strlen(question)-1]='\0';
    			printf("%s| \n",question);
    			length=strlen(question)+1;
    			
    			for(int k=0;k<4;k++){
					if(write(players[k].cl,&length,sizeof(length))<0){
						 printf("[Thread %d]\n",idThread);
                     	 perror ("Eroare la trimiterea length\n");
						}
    			    if(write(players[k].cl,question,length)<0)
    					 {printf("[Thread %d]\n",idThread);
                      	 perror ("Eroare la trimiterea intrebarii , client.\n");
 					     printf("column %d \n",i);   		        
    		         	}
    		     }
   			}
}

int callback(void* existent_acc,int count,char** data,char** columnNames){

  if(data[0]!=NULL)
  { *(int*)existent_acc=1;
   printf("Cont existent\n");}
   else { *(int*)existent_acc=0;
   printf("Cont inexistent\n");}
   

return 0;
}

void login (int cl,int idThread,sqlite3* dbUsers){
  char username[20];
  char password[10]; 
  char query[90]; 
  int logged=0;

        while(logged==0){

            if (read (cl, username,sizeof(username)) <= 0)
            {
                 printf("[Thread %d]\n",idThread);
                 perror ("Eroare la read() de la client.\n");
      
            }
           printf("numele introdus %s|",username);
            if (read (cl, password,sizeof(password) )<= 0)
              {
                 printf("[Thread %d]\n",idThread);
                perror ("Eroare la read() de la client.\n");
      
              }
            printf("parola introdusa %s|",password);

            //sqlite query

            sprintf(query,"select username ,password from users where username='%s' and password='%s';",username,password);
            printf("interogam tabela\n");

            if(sqlite3_exec(dbUsers,query,callback,&logged,NULL)!=0)
             {
                fprintf(stderr, "Can't execute query on accounts database: %s\n", sqlite3_errmsg(dbUsers));
                sqlite3_close(dbUsers);
                   
             }

         
          //se trimite clientului daca logarea s-a executat
          if(write(cl,&logged,sizeof(int))<=0){
                 printf("[Thread %d]\n",idThread);
                 perror ("Eroare la write() de la client.\n");
      
              }
      }
    char rank[15];
    int nrGames,nrVictories;
    info( dbUsers,rank,&nrGames,&nrVictories, username);
    	  	     

	if(sqlite3_close(dbUsers)!=0)
        fprintf(stderr, "Can't close the database: %s\n", sqlite3_errmsg(dbUsers));
    

     addClient_WaitingList(username, cl,rank,nrGames,nrVictories,idThread);
     
}
void info(sqlite3* dbUsers,char rank[15],int * nrGames,int *nrVictories,char username[20]){

	char query[100];
	sprintf(query,"select degree , nrgames, nrvictories from users where username like '%s';",username);
  	sqlite3_stmt *getRank;
  	int errorR=sqlite3_prepare_v2(dbUsers,query,-1,&getRank,0);
        		
     if(errorR!=SQLITE_OK)
   	    {   fprintf(stderr, "Can't get data %s\n", sqlite3_errmsg(dbUsers));
        	sqlite3_close(dbUsers);
        }

   		
    errorR=sqlite3_step(getRank);  
    
    if(errorR==SQLITE_ROW)
    strcpy(rank,sqlite3_column_text(getRank,0));
 	*nrGames=sqlite3_column_int64(getRank,1);
 	*nrVictories=sqlite3_column_int64(getRank,2);
    sqlite3_finalize(getRank);

}
void registration(int cl,int idThread,sqlite3* dbUsers){
  char username[20];
  char password[10];  
  int logged=0;
  char query[100];

  while(logged==0){

    
           if (read (cl, username,sizeof(username)) <= 0)
            {
                 printf("[Thread %d]\n",idThread);
                 perror ("Eroare la read() de la client.\n");
      
            }
           printf("numele introdus %s|",username);

            if (read (cl, password,sizeof(password) )<= 0)
              {
                 printf("[Thread %d]\n",idThread);
                 perror ("Eroare la read() de la client.\n");
      
               }
           printf("parola introdusa %s|",password);
            //verific daca mai exista un cont cu numele introdus
            sprintf(query,"select username ,password from users where username='%s' and password='%s';",username,password);
           // printf("interogam tabela\n");
            
            if(sqlite3_exec(dbUsers,query,callback,&logged,NULL)!=0)
             {
                fprintf(stderr, "Can't execute query on accounts database: %s\n", sqlite3_errmsg(dbUsers));
                sqlite3_close(dbUsers);
                   
             }
              //printf("usedName=%d\n",logged);
              fflush(stdout);
             if(logged==0)
         {   sprintf(query,"insert into users values('%s','%s',%d,%d,'beginner');",username,password,0,0);
             //printf("introduc noul user \n");
             if(sqlite3_exec(dbUsers,query,NULL,NULL,NULL)==0)
              logged=1;
            else
             {
                fprintf(stderr, "Can't introduce new user in accounts database: %s\n", sqlite3_errmsg(dbUsers));
                sqlite3_close(dbUsers);
                   
             }
          }
            //trimit raspuns clientului:0-inregistrare esuata,mai exista un cont ,1-da
            if(write(cl,&logged,sizeof(int))<=0){
                 printf("[Thread %d]\n",idThread);
                 perror ("Eroare la write() de la client.\n");
      
              }


        }
       addClient_WaitingList(username,cl,"beginner",0,0, idThread);
}
void addClient_WaitingList(char username[20],int cl,char rank[15],int nrGames,int nrVictories,int idThread){

	pthread_mutex_lock(&glock);
    printf("Thread:%d LOCK\n",idThread);
    printf("Thread %d adds a new client on the wainting list\n",idThread);

    strcpy(playersPool[waitingPlayers].username,username);
    playersPool[waitingPlayers].cl=cl;
    playersPool[waitingPlayers].nrgames=nrGames;
    playersPool[waitingPlayers].victories=nrVictories;

    strcpy(playersPool[waitingPlayers].rank,rank);


    nrPlayers++;   
    waitingPlayers++;

      if(waitingPlayers==4)
      {  printf("We're getting ready to start the game\n");
         player newgame[4];

         for(int j=0;j<=3;j++)
                  {
                    newgame[j].cl=playersPool[j].cl;
                    newgame[j].nrgames=playersPool[j].nrgames;
                    newgame[j].victories=playersPool[j].victories;
                    strcpy(newgame[j].username, playersPool[j].username);
                    strcpy(newgame[j].rank, playersPool[j].rank);
                     
                   }
        waitingPlayers=0;
        pthread_mutex_unlock(&glock);

        printf("Thread:%d UNLOCK\n",idThread);
        game(newgame,idThread);
      }
      else{ printf("Thread:%d UNLOCK\n",idThread);
            pthread_mutex_unlock(&glock);
           }       		

}

void raspunde(int cl,int idThread){
//printf("procesare logare\n");
      
        int modLog;// 1- log in ,2-registration 
        sqlite3 *dbUsers;
        
        	//mesajul primit de trimis la client  
        //Deschidem baza de date
        if(sqlite3_open("accounts.db",&dbUsers)!=0){

          fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(dbUsers));
          sqlite3_close(dbUsers);
                }
    
        //printf("am deschis baza de date\n");
        //printf("astept modul inregistrarii\n");
        if (read (cl, &modLog,sizeof(modLog)) <= 0)
            {
                 printf("[Thread %d]\n",idThread);
                 perror ("Eroare la read() de la client.\n");
      
            }
        //printf("MODUL LOGARII ESTE %d\n", modLog);
        if(modLog==1)
        { //printf("login norma\n");
          login(cl,idThread,dbUsers);
         }
      else 
        if(modLog==2)
        { //printf("login inregistrare\n");
          registration(cl,idThread,dbUsers);

        }		
}


