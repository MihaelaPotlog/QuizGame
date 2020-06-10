#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <poll.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>

#define nrQuestions 2
#define nrRounds 1
/* codul de eroare returnat de anumite apeluri */
extern int errno;
/* portul de conectare la server*/
int port;
int sd;
void connect_to_Server();
void login();
void start_game();
void read_Question(char question[5][400]);
int  stopWatch_Read(int milisec);
void wait_game();


int main (int argc, char *argv[])
{
  connect_to_Server();
  
  login();//autentificarea pentru a juca quizgame

  wait_game();
 } 
  
 void wait_game(){

  	char message[20];
	printf("Asteptam inceperea jocului\n");
	int nrBytes=read (sd, message,sizeof(message));
	if (nrBytes < 0)
    {
      perror ("[client]Eroare la read() de la server.\n");
      
    }
    else printf("s-au citit %d bytes",nrBytes);
	printf("Mesajul primit este:%s",message);
    
    start_game();
 
    }

void start_game()
{   int ok;
	int score=0;
  	char question[5][400];
  	int corect;
  	char adversari[3][20];
  	char ranks[3][15];
  	printf("Opponents:\n");
    for(int i=0;i<3;i++)
    {	if(read (sd, adversari[i],sizeof(adversari[i]))<0)
    		perror("Eroare la read()->numele adversarilor\n");
    	else printf("%s->",adversari[i]);

    	if(read (sd, ranks[i],sizeof(ranks[i]))<0)
    		perror("Eroare la read()->rangul adversarilor\n");
    	else printf("%s  | ",ranks[i]);
	}

    for(int indRound=0;indRound<nrRounds;indRound++){
    	for(int indQ=0;indQ<nrQuestions;indQ++){
    		
  		  	read_Question(question);

    		if(read(sd,&ok,sizeof(int))<0)
		    perror("Eroare la read\n");

			printf("\n%s\n",question[0]);
			for(int i=1;i<5;i++)
			printf("\t\t %s\n",question[i]);


	  		printf("Introduceti raspunsul:");
       		stopWatch_Read(25000);

       		if(read(sd,&corect,sizeof(int))<0)
       			 perror("Eroare la citirea raspunsului\n");

			if(corect==1)
				printf("Raspuns corect\n");
			else printf("Raspuns incorect\n");
			
       		score=score+corect;
         
       
        }
    }
    
    printf("SFARSIT JOC\n");
    int place;
    char name[20];
    int points;
    
    for(int i=0;i<4;i++)
    {	printf("%d place:",i+1);
    	if(read(sd,name,sizeof(name))<0)
    		perror("Eroare la citirea clasamentului\n");
    	printf("%s ",name);

    	if(read(sd,&points,sizeof(int))<0)
    		perror("Eroare la citirea clasamentului\n");
    	printf("%d \n",points);
    }
    
    printf("Congratulations!!\n");
  	char rang[15];
  	if(read(sd,rang,sizeof(rang))<0)
    	perror("Eroare la primirea rangului\n");
    printf("Your rank:%s\n",rang);
    printf("Press:\n 0->Quit.\n 1->Start a new game.");
     if(stopWatch_Read(10000)==1)
     	wait_game();
   
}
void read_Question(char question[5][400]){

	int bytesToRead,readBytes;
	char aux[100];
	aux[0]='\0';
    
	    	
  		for(int j=0;j<5;j++)
  		{   
	    	question[j][0]='\0';
  			
  			if(read(sd,&bytesToRead,sizeof(bytesToRead))<0)
  			{
     	 	perror ("[client]Eroare la citirea intrebarii  de la server.\n");
      
    		}
    		
    		//printf("%d \n",bytesToRead);

    		while(bytesToRead>0)
  			{	if((readBytes=read(sd,aux,bytesToRead))<0)
  				{
     	 		perror ("[client]Eroare la citirea intrebarii  de la server.\n");
      
    			}
    			
    			bytesToRead=bytesToRead-readBytes;
    			strcat(question[j],aux);
    			aux[0]='\0';
    		}
    	}


}

    
int  stopWatch_Read(int milisec){
	
	int answer=0;

    struct pollfd fds[1];
    fds[0].fd=0;
    fds[0].events=0;
    fds[0].events|=POLLIN;
   	int timeout=milisec;
   

    int val=poll(fds,1,timeout);

    if(val==0){
    	
     	answer=0;
     	 }
    else{
    		 scanf("%d",&answer);
    		 printf("am introdus %d\n",answer);
    	}
      
    //printf("timeout\n");
  
    if(write(sd,&answer,sizeof(answer))<0)
    	perror("Eroare la trimiterea raspunsului \n");
    return answer;

}

void connect_to_Server(){
   		// descriptorul de socket
    struct sockaddr_in server;	// structura folosita pentru conectare 
  		// mesajul trimis
    port = atoi ("2909");


  /* cream socketul */
  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("Eroare la socket().\n");
      }

    /* umplem structura folosita pentru realizarea conexiunii cu serverul */
  /* familia socket-ului */
  server.sin_family = AF_INET;
  /* adresa IP a serverului */
  server.sin_addr.s_addr = inet_addr("127.0.0.1");
  /* portul de conectare */
  server.sin_port = htons (port);
   /* ne conectam la server */
  if (connect (sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1)
    {
      perror ("[client]Eroare la connect().\n");
      
    }


  
}



void login(){
	char username[20];
  char password[10];



   int mod;
  printf("1-Login\n2-Register\n");
  scanf("%d",&mod);
  //printf("modul este %d \n",mod);
  if (write (sd,&mod,sizeof(mod)) <= 0)
    {
      perror ("[client]Eroare la write() pentru modul logarii spre server.\n");
      
    }

  int logged=0;
  int length;
  while(logged==0){
  /* citirea mesajului */
  printf ("[client]Introduce username:");
  fflush (stdout);
  length=read (0, username, sizeof(username));
  username[length-1]='\0';
  //printf("[client] Am citit %s|\n",username);
  fflush (stdout);
  //*****8
   printf ("[client]:Introduce password:");
  fflush (stdout);
  length=read (0, password, sizeof(password));
  password[length-1]='\0';
  
  
  //printf("[client] Am citit %s|\n",password);
  
  /* trimiterea mesajului la server */
  if (write (sd,username,sizeof(username)) <= 0)
    {
      perror ("[client]Eroare la write() pentru username spre server.\n");
      
    }
    if (write (sd,password,sizeof(password)) <= 0)
    {
      perror ("[client]Eroare la write() pentru parola spre server.\n");
      
    }


  /* citirea raspunsului dat de server 
     (apel blocant pina cind serverul raspunde) */
  if (read (sd, &logged,sizeof(int)) < 0)
    {
      perror ("[client]Eroare la read() de la server.\n");
      
    }
  /* afisam mesajul primit */
    if(logged==1)
  printf ("[client]Logged\n");
else printf("[client]Wrong password or username\n");
}
}