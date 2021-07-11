# _Semaphore POSIX_

## Tópicos
* [Introdução](#introdução)
* [Systemcalls](#systemcalls)
* [Implementação](#implementação)
* [posix_semaphore.h](#posix_semaphoreh)
* [posix_semaphore.c](##posix_semaphorec)
* [launch_processes.c](#launch_processesc)
* [button_interface.h](#button_interfaceh)
* [button_interface.c](#button_interfacec)
* [led_interface.h](#led_interfaceh)
* [led_interface.c](#led_interfacec)
* [Compilando, Executando e Matando os processos](#compilando-executando-e-matando-os-processos)
* [Compilando](#compilando)
* [Clonando o projeto](#clonando-o-projeto)
* [Selecionando o modo](#selecionando-o-modo)
* [Modo PC](#modo-pc)
* [Modo RASPBERRY](#modo-raspberry)
* [Executando](#executando)
* [Interagindo com o exemplo](#interagindo-com-o-exemplo)
* [MODO PC](#modo-pc-1)
* [MODO RASPBERRY](#modo-raspberry-1)
* [Matando os processos](#matando-os-processos)
* [Conclusão](#conclusão)
* [Referência](#referência)

## Introdução
POSIX Semaphore é uma padronização desse recurso para que fosse altamente portável entre os sistemas. Não difere tanto da Semaphore System V, é normalmente usado para sincronização de Threads.

## Systemcalls
Para utilizar a API referente ao Semaphore é necessário realizar a linkagem com a biblioteca pthread

Esta função cria ou abre um semaphore existente
```c
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <semaphore.h>

sem_t *sem_open(const char *name, int oflag);
sem_t *sem_open(const char *name, int oflag, mode_t mode, unsigned int value);
```

Esta função incrementa o semaphore, permitindo assim que outro processo ou thread que estejam em estado de wait possa realizar o _lock_
```c
#include <semaphore.h>

int sem_post(sem_t *sem);
```

Essa função decrementa o semaphore apontado pelo descritor.
```c
#include <semaphore.h>

int sem_wait(sem_t *sem);

int sem_trywait(sem_t *sem);

int sem_timedwait(sem_t *sem, const struct timespec *abs_timeout);
```
Fecha o semaphore, permitindo a liberação do recurso alocado para o seu uso
```c
#include <semaphore.h>

int sem_close(sem_t *sem);
```
Remove o semaphore imediatamente, o semaphore é destruído quando todos os processos que o utiliza fecha.
```c
#include <semaphore.h>

int sem_unlink(const char *name);
```

## Implementação
Para facilitar o uso desse mecanismo, o uso da API referente a Semaphore POSIX é feita através de uma abstração na forma de uma biblioteca.

### *posix_semaphore.h*
Para o seu uso é criado um estrutura que guarda o seu contexto, e o nome do semaphore
```c
typedef struct 
{
    void *handle;
    const char *name;
} POSIX_Semaphore;
```

As ações pertinentes ao semaphore permitem criar, pegar, liberar, esperar e liberar o recurso.
```c
bool POSIX_Semaphore_Create(POSIX_Semaphore *semaphore);
bool POSIX_Semaphore_Get(POSIX_Semaphore *semaphore);
bool POSIX_Semaphore_Post(POSIX_Semaphore *semaphore);
bool POSIX_Semaphore_Wait(POSIX_Semaphore *semaphore);
bool POSIX_Semaphore_Cleanup(POSIX_Semaphore *semaphore);
```

### *posix_semaphore.c*
Aqui em POSIX_Semaphore_Create criamos o semaphore baseado no nome e seu contexto e guardado em handle.

```c
bool POSIX_Semaphore_Create(POSIX_Semaphore *semaphore)
{
    bool status = false;

    do 
    {
        if(!semaphore)
            break;

        semaphore->handle = sem_open(semaphore->name, O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO, 0);
        if(!semaphore->handle)
            break;

        status = true;
    } while(false);

    return status;
}
```
Na POSIX_Semaphore_Get é verificado se o semaphore está disponível para uso.
```c
bool POSIX_Semaphore_Get(POSIX_Semaphore *semaphore)
{
    bool status = false;

    do 
    {
        if(!semaphore)
            break;

        if(sem_post(semaphore->handle) == 0)
            status = true;

        semaphore->handle = sem_open(semaphore->name, O_RDWR);
        if(!semaphore->handle)
            break;

        status = true;
        
    } while(false);

    return status;
}
```
Incrementa o semaphore para o habilitar seu uso
```c
bool POSIX_Semaphore_Post(POSIX_Semaphore *semaphore)
{
    bool status = false;
    if(semaphore && semaphore->handle)
    {
        if(sem_post(semaphore->handle) == 0)
            status = true;
    }

    return status;
}
```
Aguarda que o semaphore seja liberado para ser usado
```c
bool POSIX_Semaphore_Wait(POSIX_Semaphore *semaphore)
{
    bool status = false;
    if(semaphore && semaphore->handle)
    {
        if(sem_wait(semaphore->handle) == 0)
            status = true;
    }
    return status;
}
```

Libera os recursos alocados para o semaphore
```c
bool POSIX_Semaphore_Cleanup(POSIX_Semaphore *semaphore)
{
    bool status = false;
    if(semaphore && semaphore->handle)
    {
        sem_close(semaphore->handle);
        sem_unlink(semaphore->name);
        status = true;
    }

    return status;
}
```

Para demonstrar o uso desse IPC, iremos utilizar o modelo Produtor/Consumidor, onde o processo Produtor(_button_process_) vai escrever seu estado interno no arquivo, e o Consumidor(_led_process_) vai ler o estado interno e vai aplicar o estado para si. Aplicação é composta por três executáveis sendo eles:
* _launch_processes_ - é responsável por lançar os processos _button_process_ e _led_process_ através da combinação _fork_ e _exec_
* _button_interface_ - é responsável por ler o GPIO em modo de leitura da Raspberry Pi e escrever o estado interno no arquivo
* _led_interface_ - é responsável por ler do arquivo o estado interno do botão e aplicar em um GPIO configurado como saída

### *launch_processes.c*

No _main_ criamos duas variáveis para armazenar o PID do *button_process* e do *led_process*, e mais duas variáveis para armazenar o resultado caso o _exec_ venha a falhar.
```c
int pid_button, pid_led;
int button_status, led_status;
```

Em seguida criamos um processo clone, se processo clone for igual a 0, criamos um _array_ de *strings* com o nome do programa que será usado pelo _exec_, em caso o _exec_ retorne, o estado do retorno é capturado e será impresso no *stdout* e aborta a aplicação. Se o _exec_ for executado com sucesso o programa *button_process* será carregado. 
```c
pid_button = fork();

if(pid_button == 0)
{
    //start button process
    char *args[] = {"./button_process", NULL};
    button_status = execvp(args[0], args);
    printf("Error to start button process, status = %d\n", button_status);
    abort();
}   
```

O mesmo procedimento é repetido novamente, porém com a intenção de carregar o *led_process*.

```c
pid_led = fork();

if(pid_led == 0)
{
    //Start led process
    char *args[] = {"./led_process", NULL};
    led_status = execvp(args[0], args);
    printf("Error to start led process, status = %d\n", led_status);
    abort();
}
```

### *button_interface.h*
Para usar a interface do botão precisa implementar essas duas callbacks para permitir o seu uso.
```c
typedef struct 
{
    bool (*Init)(void *object);
    bool (*Read)(void *object);
    
} Button_Interface;
```
A assinatura do uso da interface corresponde ao contexto do botão, que depende do modo selecionado, o contexo do Semaphore, e a interface do botão devidamente preenchida.
```c
bool Button_Run(void *object, POSIX_Semaphore *semaphore, Button_Interface *button);
```

### *button_interface.c*
A implementação da interface baseia-se em inicializar o botão, inicializar o Semaphore, e no loop incrementa o semaphore mediante o pressionamento do botão.
```c
bool Button_Run(void *object, POSIX_Semaphore *semaphore, Button_Interface *button)
{
    if(button->Init(object) == false)
		return false;

    if(POSIX_Semaphore_Create(semaphore) == false)
        return false;

    while(true)
	{
        wait_press(object, button);
        POSIX_Semaphore_Post(semaphore);
	}

    POSIX_Semaphore_Cleanup(semaphore);
   
    return false;
}
```

### *led_interface.h*
Para realizar o uso da interface de LED é necessário preencher os callbacks que serão utilizados pela implementação da interface, sendo a inicialização e a função que altera o estado do LED.
```c
typedef struct 
{
    bool (*Init)(void *object);
    bool (*Set)(void *object, uint8_t state);
} LED_Interface;
```

A assinatura do uso da interface corresponde ao contexto do LED, que depende do modo selecionado, o contexo do Semaphore, e a interface do LED devidamente preenchida.
```c
bool LED_Run(void *object, POSIX_Semaphore *semaphore, LED_Interface *led);
```

### *led_interface.c*
A implementação da interface baseia-se em inicializar o LED, inicializar o Semaphore, e no loop verifica se há semaphore disponível para poder alterar o seu estado.
```c
bool LED_Run(void *object, POSIX_Semaphore *semaphore, LED_Interface *led)
{
	int status = 0;

	if(led->Init(object) == false)
		return false;

	if(POSIX_Semaphore_Create(semaphore) == false)
		return false;		

	while(true)
	{
		if(POSIX_Semaphore_Wait(semaphore) == true)
		{
			status ^= 0x01; 
			led->Set(object, status);
		}
	}

	return false;	
}
```

## Compilando, Executando e Matando os processos
Para compilar e testar o projeto é necessário instalar a biblioteca de [hardware](https://github.com/NakedSolidSnake/Raspberry_lib_hardware) necessária para resolver as dependências de configuração de GPIO da Raspberry Pi.

## Compilando
Para faciliar a execução do exemplo, o exemplo proposto foi criado baseado em uma interface, onde é possível selecionar se usará o hardware da Raspberry Pi 3, ou se a interação com o exemplo vai ser através de input feito por FIFO e o output visualizado através de LOG.

### Clonando o projeto
Pra obter uma cópia do projeto execute os comandos a seguir:

```bash
$ git clone https://github.com/NakedSolidSnake/Raspberry_IPC_Semaphore_POSIX
$ cd Raspberry_IPC_Semaphore_POSIX
$ mkdir build && cd build
```

### Selecionando o modo
Para selecionar o modo devemos passar para o cmake uma variável de ambiente chamada de ARCH, e pode-se passar os seguintes valores, PC ou RASPBERRY, para o caso de PC o exemplo terá sua interface preenchida com os sources presentes na pasta src/platform/pc, que permite a interação com o exemplo através de FIFO e LOG, caso seja RASPBERRY usará os GPIO's descritos no [artigo](https://github.com/NakedSolidSnake/Raspberry_lib_hardware#testando-a-instala%C3%A7%C3%A3o-e-as-conex%C3%B5es-de-hardware).

#### Modo PC
```bash
$ cmake -DARCH=PC ..
$ make
```

#### Modo RASPBERRY
```bash
$ cmake -DARCH=RASPBERRY ..
$ make
```

## Executando
Para executar a aplicação execute o processo _*launch_processes*_ para lançar os processos *button_process* e *led_process* que foram determinados de acordo com o modo selecionado.

```bash
$ cd bin
$ ./launch_processes
```

Uma vez executado podemos verificar se os processos estão rodando atráves do comando:
```bash
$ ps -ef | grep _process
```

O output:
```bash
cssouza  16871  3449  0 07:15 pts/4    00:00:00 ./button_process
cssouza  16872  3449  0 07:15 pts/4    00:00:00 ./led_process
```
## Interagindo com o exemplo
Dependendo do modo de compilação selecionado a interação com o exemplo acontece de forma diferente.

### MODO PC
Para o modo PC, precisamos abrir um terminal e monitorar os LOG's.
```bash
$ sudo tail -f /var/log/syslog | grep LED
```

Dessa forma o terminal irá apresentar somente os LOG's referente ao exemplo.

Para simular o botão, o processo em modo PC cria uma FIFO para permitir enviar comandos para a aplicação, dessa forma todas as vezes que for enviado o número 0 irá logar no terminal onde foi configurado para o monitoramento, segue o exemplo
```bash
echo "0" > /tmp/sema_posix_fifo
```

Output do LOG quando enviado o comando algumas vezez
```bash
Apr 16 16:51:26 cssouza-Latitude-5490 LED SEMAPHORE POSIX[29345]: LED Status: Off
Apr 16 16:51:26 cssouza-Latitude-5490 LED SEMAPHORE POSIX[29345]: LED Status: On
Apr 16 16:51:26 cssouza-Latitude-5490 LED SEMAPHORE POSIX[29345]: LED Status: Off
Apr 16 16:51:49 cssouza-Latitude-5490 LED SEMAPHORE POSIX[29345]: LED Status: On
Apr 16 16:51:51 cssouza-Latitude-5490 LED SEMAPHORE POSIX[29345]: LED Status: Off
Apr 16 16:51:51 cssouza-Latitude-5490 LED SEMAPHORE POSIX[29345]: LED Status: On
```

### MODO RASPBERRY
Para o modo RASPBERRY a cada vez que o botão for pressionado irá alternar o estado do LED.

## Matando os processos
Para matar os processos criados execute o script kill_process.sh
```bash
$ cd bin
$ ./kill_process.sh
```

## Conclusão
POSIX Semaphore é um modo moderno de realizar sincronização entre os acessos às partes críticas do sistema, diferente do Semaphore System V é largamente utilizado para a sincronização de Threads. De qualquer forma é uma alternativa ao Semaphore System V.

## Referência
* [Link do projeto completo](https://github.com/NakedSolidSnake/Raspberry_IPC_Semaphore_POSIX)
* [Mark Mitchell, Jeffrey Oldham, and Alex Samuel - Advanced Linux Programming](https://www.amazon.com.br/Advanced-Linux-Programming-CodeSourcery-LLC/dp/0735710430)
* [fork, exec e daemon](https://github.com/NakedSolidSnake/Raspberry_fork_exec_daemon)
* [biblioteca hardware](https://github.com/NakedSolidSnake/Raspberry_lib_hardware)

