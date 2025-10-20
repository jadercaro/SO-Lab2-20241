
# Práctica 2 de laboratorio 

# Nombre de integrantes
- Jhon Jader Caro Sanchez - CC 1001137636

# Programa Wish

## Descripción

Este programa es como un un shell (intérprete de comandos) como el que se usa en la terminal de Linux.

## Ubicación del código

El código principal se encuentra en: `./wish.c`

## Explicación de funcionalidades por bloques

### Validación de argumentos al iniciar
```bash
if(argc==2){
    archivo=fopen(argv[1],"r");
    if(!archivo){
        char error_message[30]="An error has occurred\n";
        write(STDERR_FILENO,error_message,strlen(error_message));
        exit(1);}modo_batch=1;}
   else if(argc>2){
    char error_message[30]="An error has occurred\n";
    write(STDERR_FILENO,error_message,strlen(error_message));
    exit(1);}
```
- Si se pasa 1 argumento: intenta abrir ese archivo como batch
- Si el archivo no existe o no se puede leer: error y salida
- Si se pasan más de 1 argumento: error y salida
- Sin argumentos: modo interactivo (se lee desde teclado)

### Loop principal del Shell
```bash
while(1){
    if(!modo_batch){printf("wish> ");}
    ssize_t len;
    if(modo_batch){
        len=getline(&linea,&tam,archivo);}
   else{
        len=getline(&linea,&tam,stdin);}
    if(len==-1){break;} 
    if(linea[len-1]=='\n'){linea[len-1]='\0';}
```
- Bucle infinito que solo sale con exit o EOF (End Of File)
- Imprime wish>  solo en modo interactivo
- getline(): Lee una línea completa
- Si retorna -1: significa EOF
- Elimina saltos de líneas al final de la línea

### Separación de varios comandos
```bash
char *comandos[100];
int num_comandos=0;
char *copia=strdup(linea);
char *token=strtok(copia,"&");
while(token!=NULL){
    comandos[num_comandos]=strdup(token);
    num_comandos++;
    token=strtok(NULL,"&");}
free(copia);
```
- Se divide la cadena usando & como separador
- Se guarda cada comando en el array comandos[]
- strdup(): se hace una copia de cada comando en memoria

### Procesamiento de cada comando
```bash
for(i=0;i<num_comandos;i++){
    char *cmd=comandos[i];
    while(*cmd==' '||*cmd=='\t')cmd++;
    if(strlen(cmd)==0)continue;
```
- Se recorre todos los comandos separados por &
- Se Eliminan espacios al inicio del comando
- Si el comando está vacío (solo espacios), lo salta

### Detección de redirección
```bash
char *args[100];
int num_args=0;
char *redir=NULL;
int tiene_redir=0;

char *temp=strdup(cmd);
char *parte=strtok(temp," \t");
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
```
- Se usa strtok para dividir por espacios
- Si encuentra >, todo lo que viene después es el archivo de salida
- Se guarda cada palabra en el array args[]
- El último elemento debe ser NULL

### Comando EXIT
```bash
if(strcmp(args[0],"exit")==0){
    if(num_args>1){
        char error_message[30]="An error has occurred\n";
        write(STDERR_FILENO,error_message,strlen(error_message));}
   else{exit(0);}
    continue;}
```
- Compara si el comando es exactamente "exit"
- Si tiene argumentos adicionales: error
- Si no tiene argumentos: termina el programa con código 0
- continue: salta al siguiente comando (no crea proceso hijo)

### Comando CD
```bash
if(strcmp(args[0],"cd")==0){
    if(num_args!=2){
        char error_message[30]="An error has occurred\n";
        write(STDERR_FILENO,error_message,strlen(error_message));}
    else{
        if(chdir(args[1])!=0){
            char error_message[30]="An error has occurred\n";
            write(STDERR_FILENO,error_message,strlen(error_message));}}
    continue;}
```
- Se verifica que tenga exactamente 1 argumento
- Se usa chdir() para cambiar de directorio
- Si chdir falla (directorio no existe): imprime error
- No crea proceso hijo (se ejecuta en el shell mismo)

### Comando PATH
```bash
if(strcmp(args[0],"path")==0){
    int j;
    for(j=0;j<num_paths;j++){if(path[j])free(path[j]);}
    num_paths=0;
    for(j=1;j<num_args;j++){
        path[num_paths]=strdup(args[j]);
        num_paths++;}
    path[num_paths]=NULL;
    continue;}
```
- Libera todas las rutas anteriores
- Copia las nuevas rutas desde args[1] en adelante
- Puede recibir 0 o más argumentos
- Si recibe 0 argumentos: el path queda vacío (no se puede ejecutar nada)

### Creación de proceso hijo
```bash
int pid=fork();
if(pid==0){...
```
- fork(): Crea una copia exacta del proceso actual
- Retorna 0 en el proceso hijo
- Retorna el PID del hijo en el proceso padre
- Todo el código dentro del if se ejecuta solo en el hijo

### Redirección de salida
```bash
if(tiene_redir&&redir!=NULL){
    int fd=open(redir,O_WRONLY|O_CREAT|O_TRUNC,0644);
    if(fd>=0){
        dup2(fd,STDOUT_FILENO);
        dup2(fd,STDERR_FILENO);
        close(fd);}}
```
- dup2(): Redirige stdout y stderr al archivo

### Búsqueda del ejecutable
```bash
char ejecutable[1000];
int encontrado=0;
int x;
for(x=0;x<num_paths;x++){
    if(path[x]==NULL)break;
    snprintf(ejecutable,sizeof(ejecutable),"%s/%s",path[x],args[0]);
    if(access(ejecutable,X_OK)==0){
        encontrado=1;
        break;}}
```
- Recorre cada directorio del path
- snprintf(): Construye la ruta completa ejemplo: /bin/ls
- access(ejecutable, X_OK): Verifica si existe y es ejecutable
- Si lo encuentra: marca como encontrado igual a 1 y sale del bucle

### Ejecución del comando
```bash
if(encontrado){execv(ejecutable,args);}
char error_message[30]="An error has occurred\n";
write(STDERR_FILENO,error_message,strlen(error_message));
exit(1);
```
- execv(): Reemplaza el proceso actual
- Si funciona: el proceso se transforma en el nuevo programa
- Si execv retorna: hubo un error (comando no existe o no se puede ejecutar)
- Si no encontró el ejecutable o execv falla: imprime error y termina el hijo

### Espera de procesos hijos
```bash
for(i=0;i<num_comandos;i++){wait(NULL);}
```
- Se ejecuta solo en el proceso padre
- wait(NULL): Bloquea hasta que UN hijo termine
- Se llama tantas veces como comandos se lanzaron
- Evita procesos zombie

## Compilación

Para compilar el programa:

```bash
gcc -o wish ./wish.c
```

## Ejemplos de uso

### Ejemplo 1: Uso básico
```bash
./wish
wish> ls
archivo1.txt  archivo2.c  wish
wish> pwd
/home/usuario
wish> exit

```

### Ejemplo 2: Redirección
```bash
./wish
wish> echo "Hola mundo" > salida.txt
wish> cat salida.txt
Hola mundo
wish> ls -la > archivos.txt
wish> cat archivos.txt
total 48
-rw-r--r-- 1 user user  1234 Oct 19 archivo1.txt
-rw-r--r-- 1 user user  5678 Oct 19 archivo2.c
wish> exit
```

### Ejemplo 3: Comandos paralelos
```bash
./wish
wish> echo uno & echo dos & echo tres
uno
dos
tres
wish> sleep 1 & sleep 1 & sleep 1
[espera 1 segundo, no 3]
wish> exit
```
### Ejemplo 4: Cambio de path
```bash
./wish
wish> ls
archivo.txt
wish> path /usr/bin /bin
wish> ls
archivo.txt
wish> python3 --version
Python 3.8.10
wish> path
wish> ls
An error has occurred
wish> cd /tmp
An error has occurred
wish> exit
An error has occurred
```

### Ejemplo 5: Comandos paralelos
```bash
cat > prueba.txt
ls
pwd
echo "Hola Mundo y UdeA"

-----
./wish prueba.txt
archivo1.txt  archivo2.c  wish
/home/usuario
Hola Mundo y UdeA
```
