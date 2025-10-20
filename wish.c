//Importación de librerías necesarias
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

int main(int argc, char *argv[]){
FILE *archivo=NULL;
char *linea=NULL;
size_t tam=0;
int modo_batch=0;

//Si hay dos argumentos, se abre el archivo en modo batch
if(argc==2){
archivo=fopen(argv[1],"r");
//Si falla la apertura del archivo, se muestra un mensaje de error y se sale
if(!archivo){
char error_message[30]="An error has occurred\n";
write(STDERR_FILENO,error_message,strlen(error_message));
exit(1);}
modo_batch=1;}

//Si hay más de dos argumentos, se muestra un mensaje de error y se sale
else if(argc>2){
char error_message[30]="An error has occurred\n";
write(STDERR_FILENO,error_message,strlen(error_message));
exit(1);}

//Inicialización del path con /bin por defecto
char *path[100];
path[0]=strdup("/bin");
path[1]=NULL;
int num_paths=1;

//Ejecución del loop principal
while(1){
if(!modo_batch){
printf("wish> ");}

ssize_t len;
//Lectura de la línea de entrada dependiendo del modo
if(modo_batch){
len=getline(&linea,&tam,archivo);}
else{
len=getline(&linea,&tam,stdin);}

//Si se llega al final del archivo, se sale del loop
if(len==-1){
break;}

//Eliminación del salto de línea al final de la línea
if(linea[len-1]=='\n'){
linea[len-1]='\0';}

char *comandos[100];
int num_comandos=0;
char *copia=strdup(linea);
char *token=strtok(copia,"&");

//Separación de los comandos por el operador & 
while(token!=NULL){
comandos[num_comandos]=strdup(token);
num_comandos++;
token=strtok(NULL,"&");}
free(copia);

int i;

//Se procesan cada comando por separado
for(i=0;i<num_comandos;i++){
char *cmd=comandos[i];
while(*cmd==' '||*cmd=='\t')cmd++;
if(strlen(cmd)==0)continue;

char *args[100];
int num_args=0;
char *redir=NULL;
int tiene_redir=0;

char *temp=strdup(cmd);
char *parte=strtok(temp," \t");

//Se separan los argumentos del comando y se detecta redirección
while(parte!=NULL){
if(strcmp(parte,">")==0){
tiene_redir=1;
parte=strtok(NULL," \t");
if(parte!=NULL){redir=strdup(parte);}
break;}
args[num_args]=strdup(parte);
num_args++;
parte=strtok(NULL," \t");}

free(temp);
args[num_args]=NULL;

//Si no hay argumentos, se continúa con el siguiente comando
if(num_args==0)continue;

//Si el comando es exit, se maneja la salida
if(strcmp(args[0],"exit")==0){
if(num_args>1){
char error_message[30]="An error has occurred\n";
write(STDERR_FILENO,error_message,strlen(error_message));}
else{
exit(0);}
continue;}

//Si el comando es cd, se maneja el cambio de directorio
if(strcmp(args[0],"cd")==0){
if(num_args!=2){
char error_message[30]="An error has occurred\n";
write(STDERR_FILENO,error_message,strlen(error_message));}
else{
if(chdir(args[1])!=0){
char error_message[30]="An error has occurred\n";
write(STDERR_FILENO,error_message,strlen(error_message));}}
continue;}

//Si el comando es path, se maneja la modificación del path
if(strcmp(args[0],"path")==0){
int j;
for(j=0;j<num_paths;j++){if(path[j])free(path[j]);}
num_paths=0;
for(j=1;j<num_args;j++){
path[num_paths]=strdup(args[j]);
num_paths++;}
path[num_paths]=NULL;
continue;}

int pid=fork();
//Ejecución del comando en un proceso hijo
if(pid==0){
if(tiene_redir&&redir!=NULL){
int fd=open(redir,O_WRONLY|O_CREAT|O_TRUNC,0644);
if(fd>=0){
dup2(fd,STDOUT_FILENO);
dup2(fd,STDERR_FILENO);
close(fd);}}

char ejecutable[1000];
int encontrado=0;
int x;
//Se busca el ejecutable en las rutas del path
for(x=0;x<num_paths;x++){
if(path[x]==NULL)break;
//Se Construye la ruta completa del ejecutable
snprintf(ejecutable,sizeof(ejecutable),"%s/%s",path[x],args[0]);
if(access(ejecutable,X_OK)==0){
encontrado=1;
break;}}

//Si se encuentra el ejecutable, se ejecuta
if(encontrado){
execv(ejecutable,args);}
char error_message[30]="An error has occurred\n";
write(STDERR_FILENO,error_message,strlen(error_message));
exit(1);}}

for(i=0;i<num_comandos;i++){
wait(NULL);}}

if(modo_batch){fclose(archivo);}
free(linea);
return 0;}