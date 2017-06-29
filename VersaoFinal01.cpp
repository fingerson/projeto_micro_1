/*
 * SISTEMA DE CONTROLE E AUTOMAÇÃO RESIDENCIAL
	* Comunicação por Bluetooth
		* Interface humana através de um celular.
		* O microcontrolador se comunica com o celular através de um módulo Bluetooth HC-05
	* Sistema de acionamento condicional
		* O microcontrolador aciona dispositivos (lâmpadas, dispositivos ativados eletricamente em geral)
		de acordo com condições estabelecidas pelo usuário (tempo, valor de tensão, etc).
		*Pode administrar até 6 dispositivos e sensores diferentes.
*/

// Definindo as portas de entrada e saída dos respectivos processos
const int portaOut[6]={3,5,6,9,10,11}; 
const int portaIn[6]={A0,A1,A2,A3,A4,A5};

int start; //variável que é setada quando o programa inicia.

struct pedido // struct de formulário que indica quais informações chegaram pelo bluetooth
{
  char tipo;// Tipos de formulários possíveis (b) vazio, (s) configurar, (r) retornar, (k) destruir, (a) ativar;
  int ID; // Indica o processo em questão (0 a 6)
  unsigned long valorLigar; // valor necessário para que o pino de saída do processo vá de 0 a 1
  unsigned long valorDesligar; // valor necessário para que o pino de saída do processo vá de 1 a 0
  int condLigar; // condição para que o pino de saída do processo vá de 0 a 1 (0 - temporização, 1 - pulso de entrada positivo, 2 - pulso de entrada negativo, 3 - threshold positivo, 4 - threshold negativo)
  int condDesligar; // condição para que o pino de saída do processo vá de 1 a 0 (o mesmo que dito acima)
  int estadoInicial; // estano no qual o processo se inicia (0 ou 1)
};

struct processos // Struct que armazena os dados dos processos concorrentes.
{
  int estaAtivo; // Indica se o processo está ativo
  unsigned long valorLigar; // valor necessário para que o pino de saída do processo vá de 0 a 1
  unsigned long valorDesligar; // valor necessário para que o pino de saída do processo vá de 1 a 0
  int condLigar; // condição para que o pino de saída do processo vá de 0 a 1  (0 - temporização, 1 - pulso de entrada, 2 - threshold positivo, 3 - threshold negativo)
  int condDesligar; // condição para que o pino de saída do processo vá de 1 a 0 (o mesmo que dito acima)

  unsigned long valorAtual; // valor que indica qual o valor de transição (tanto para subida quanto decida) em que o processo se encontra.
  int estadoAtual; // Indica se o pino de saída do processo está ligado ou desligado.

  unsigned long ultimaInteracao; // Utilizado na comparação de tempo para indicar quando ocorreu a última medida
  unsigned long correcao; // Utilizado para corrigir os possíveis erros na comparação de tempo.
  int bandeira; // flag que indica se o botão 
};


int charToInt (char a) // Função que converte um algarismo char para int
{
  if(a=='0')                                                                              
    return 0;
  if(a=='1')
    return 1;
  if(a=='2')
    return 2;
  if(a=='3')
    return 3;
  if(a=='4')
    return 4;
  if(a=='5')
    return 5;
  if(a=='6')
    return 6;
  if(a=='7')
    return 7;
  if(a=='8')
    return 8;
  if(a=='9')
    return 9;
  return 0;
}

int indexRequest=0; // Variáveis que armazenam o input do bluetooth
char command[30];
pedido getRequest() //Função que recebe os dados do bluetooth e retorna uma struct de formulário, é executada a cada loop e retorna um formulário vazio se não terminou de receber a mensagem
{
  struct pedido request;
  request.tipo='b';
  char input;
  input=Serial.read();
  command[indexRequest]=input;
  indexRequest++;
  if(input=='z') // z é o caracter que indica que o comando atual é invalido e deve ser cancelado.
  {
    indexRequest=0;
  }
  else if(input=='e') // e é o caracter que indica que o comando chegou ao fim, com isso o programa decifra o comando.
  {
    indexRequest=0;
    if(command[0]=='r' || command[0]=='k' || command[0]=='a') // caso o comando seja simples (não de configuração) a função retorna apenas um formulário somente com o tipo de comando e a ID do processo.
    {
      request.tipo=command[0]; // identifica o tipo do comando
      request.ID=charToInt(command[1]); // Identifica a ID do processo
    }
    else if (command[0]=='s') // caso o comando seja complexo (configuração) a função retorna um formulário completo
    {
      request.tipo=command[0]; // identifica o tipo do comando
      request.ID=charToInt(command[1]); // identifica a ID do processo
      int index;
      unsigned long acc=0;
      int i=3;
      request.condLigar = charToInt(command[i]); //identifica a condição de ativação da saída do processo
      i+=2;
      request.condDesligar = charToInt(command[i]); // identifica a condição de desativação da saída do processo
      i+=2;
      index=i;
      for(i=index;command[i]!='.';i++)
      {
        acc=acc*10+charToInt(command[i]); // processa o valor de ativação da saída do processo
      }
      request.valorLigar=acc; // identifica o valor de ativação da saída do processo
      i++;
      index=i;
      acc=0;
      for(i=index;command[i]!='.';i++)
      {
        acc=acc*10+charToInt(command[i]); // processa o valor de desativação da saída do processo
      }
      request.valorDesligar=acc; // identifica o valor de desativação da saída do processo
      i++;
      request.estadoInicial=charToInt(command[i]); // identifica o estado inicial do processo
    }
    else // caso não seja nenhum dos casos acima (comando inválido) retorna um formulário em branco
    {
      request.tipo='b';
    }
  }
  return request;
}

void sendData(struct processos *Proc, int ID) // Retorna o estado de um processo pelo bluetooth
{
  Serial.print("r");
  Serial.print(ID);
  Serial.print(".");
  Serial.print(Proc->estaAtivo);
  Serial.print(".");
  Serial.print(Proc->estadoAtual);
  Serial.print(".");
  Serial.print(Proc->condLigar);
  Serial.print(".");
  Serial.print(Proc->condDesligar);
  Serial.print(".");
  Serial.print(Proc->valorLigar);
  Serial.print(".");
  Serial.print(Proc->valorDesligar);
  Serial.print(".");
  Serial.print(Proc->valorAtual);
  Serial.println("e");
  
}

processos setData(struct pedido *request) // Retorna uma struct de processo com os dados recebidos pelo formulário.
{
  struct processos Proc;
  Proc.estaAtivo = 1;
  Proc.condLigar = request->condLigar;
  Proc.condDesligar = request->condDesligar;
  Proc.valorLigar = request->valorLigar;
  Proc.valorDesligar = request->valorDesligar;
  Proc.estadoAtual = request->estadoInicial;
  Proc.ultimaInteracao = millis();

  return Proc;

}

void ativar(struct processos *Proc) // Ativa ou desativa a saída do processo em questão
{
  Proc->valorAtual=0;
  Proc->estadoAtual=1-Proc->estadoAtual;
  Proc->bandeira=0;
  Proc->ultimaInteracao=millis();
}

processos destruir(int ID) // Desliga permanentemente um processo, precisando configurá-lo novamente posteriormente.
{
  struct processos Proc;
  
  Proc.estaAtivo=0;
  Proc.valorLigar=0;
  Proc.valorDesligar=0;
  Proc.condLigar=0;
  Proc.condDesligar=0;
  Proc.valorAtual=0;
  Proc.estadoAtual=0;
  Proc.ultimaInteracao=millis();
  Proc.bandeira=0;
  Proc.correcao=0;
  digitalWrite(portaOut[ID],LOW);

  return Proc;
}

void calcularTempo(struct processos *Proc) // calcula a condição de temporização de um processo
{
  unsigned long nseg=millis()+Proc->correcao-Proc->ultimaInteracao; //Calcula quantos milisegundos se passaram desde o último cálculo
  if(nseg>=1000) // Verifica se já se passou mais de 1 segundo e atualiza os valores
  {
    Proc->ultimaInteracao=millis();
    Proc->correcao=nseg%1000;
    Proc->valorAtual+=nseg/1000;    
  }  
}

void calcularPulso(struct processos *Proc, int ID, int cond) // calcula a condição de recebimento de pulsos de entrada
{
  if(digitalRead(portaIn[ID])==1 && Proc->bandeira==0) // se a leitura é 1 e antes era 0, muda a bandeira
  {
    Proc->bandeira=1;
    if(cond==2) // se o modo for de pulso negativo, soma 1 ao valor atual
      Proc->valorAtual++;
  }
  if(digitalRead(portaIn[ID])==0 && Proc->bandeira==1) // se a leitura é 0 e antes era 1, muda a bandeira
  {
    Proc->bandeira=0;
    if(cond==1) // se o modo for de pulso positivo, soma 1 ao valor atual
      Proc->valorAtual++;
  }
}

void calcularLimiar(struct processos *Proc, int ID, int cond) // calcula a condição de detecção de limiar na entrada
{
  if (cond==3)
    Proc->valorAtual=analogRead(portaIn[ID]); // se o modo for de limiar positivo, retorna o valor do sensor
  else if (cond==4)
  {
    int valorCond;
    if(Proc->estadoAtual==0)
      valorCond=Proc->valorLigar;
    else if (Proc->estadoAtual==1)
      valorCond=Proc->valorDesligar;
    Proc->valorAtual=2*valorCond-analogRead(portaIn[ID]); // se o modo for limiar negativo, retorna o valor necessário acrescido da diferença entre o valor necessário e a leitura do sensor (precisa disso por causa do proesso de ativação)
  }
}

void calcularMudanca(struct processos *Proc,int ID) // calcula as condições e realiza a mudança de estado dos processos
{
  if(Proc->estaAtivo) // verifica se o processo está habilitado
  {
    if(Proc->estadoAtual!=digitalRead(portaOut[ID])) // verifica se o estado do processo condiz com a leitura da saida 
    {
      digitalWrite(portaOut[ID],Proc->estadoAtual); // se não, muda a saída para condizer com o estado
    }
    if(Proc->estadoAtual==0) // verifica se o estado atual é desligado
    {
      if(Proc->condLigar==0) // verifica se a condição de ligar é de temporização
        calcularTempo(Proc); // calcula a condição de temporização 
      else if (Proc->condLigar==1 || Proc->condLigar==2) // verifica se a condição de ligar é de pulsos de entrada
        calcularPulso(Proc,ID,Proc->condLigar); // calcula a condição de pulsos de entrada
      else if (Proc->condLigar==3 || Proc->condLigar==4) // verifica se a condição de ligar é de detecção de limiar
        calcularLimiar(Proc,ID,Proc->condLigar); // calcula a condição de detecção de limiar
      if(Proc->valorAtual>=Proc->valorLigar) // verifica se ocorre a mudança de estado
      {
        Proc->estadoAtual=1;
        Proc->valorAtual=0;
        Proc->ultimaInteracao=millis();
        Proc->correcao=0;
        Proc->bandeira=0;
      }
    }
    if(Proc->estadoAtual==1) // verifica se o estado atual é ligado
    {
      if(Proc->condDesligar==0) // verifica se a condição de desligar é de temporização
        calcularTempo(Proc); // calcula a condição de temporização 
      else if (Proc->condDesligar==1 || Proc->condDesligar==2) // verifica se a condição de desligar é de pulsos de entrada
        calcularPulso(Proc,ID,Proc->condDesligar); // calcula a condição de pulsos de entrada
      else if (Proc->condDesligar==3 || Proc->condDesligar==4) // verifica se a condição de desligar é de detecção de limiar
        calcularLimiar(Proc,ID,Proc->condDesligar); // calcula a condição de detecção de limiar
      if(Proc->valorAtual>=Proc->valorDesligar) // verifica se ocorre a mudança de estado
      {
        Proc->estadoAtual=0;
        Proc->valorAtual=0;
        Proc->ultimaInteracao=millis();
        Proc->correcao=0;
        Proc->bandeira=0;
      }
    }
  }
}


void setup ()
{
  Serial.begin(9600); // inicia a comunicação serial
  start=1; // indica que o programa iniciou
  for(int j=0;j<6;j++) // configura as portas de input e output
  {
    pinMode(portaIn[j],INPUT);
    pinMode(portaOut[j],OUTPUT);
  }
}

void loop ()
{
  struct processos Procs[6];
  if(start==1) // é executado só no primeiro loop
  {
    start=0;
    for(int cont=0;cont<6;cont++)
    {
      Procs[cont].estaAtivo=0; // zera todos os atributos de todos os processos
      Procs[cont].valorLigar=0;
      Procs[cont].valorDesligar=0;
      Procs[cont].condLigar=0;
      Procs[cont].condDesligar=0;
      Procs[cont].valorAtual=0;
      Procs[cont].estadoAtual=0;
      Procs[cont].ultimaInteracao=millis();
      Procs[cont].bandeira=0;
      Procs[cont].correcao=0;
    }
  }
  if(Serial.available()) //detecta se chegou algum dado
  {
    struct pedido request = getRequest(); // requer formulário com informações recebidas
    if(request.tipo!='b') // verifica se o formulário não é vazio
    {
      if(request.tipo == 'r') // verifica se o formulário é de retorno
      {
        sendData(&Procs[request.ID], request.ID); // manda os valores do processo indicado pelo ID por bluetooth
      }
      else if (request.tipo == 'a') // verifica se o formulário é de ativação
      {
        ativar(&Procs[request.ID]); // ativa o processo indicado pelo ID
      }
      else if (request.tipo == 'k') // verifica se o formulário é de destruição
      {
        Procs[request.ID]=destruir(request.ID); // destrói o processo indicado pelo ID
      }
      else if (request.tipo == 's') // verifica se o formulário é de configuração
      {
        Procs[request.ID]=setData(&request); // configura o processo indicado pelo ID
        sendData(&Procs[request.ID], request.ID); // retorna os dados do processo configurado a fim de detectar possíveis erros de configuração
      }
    }
  }
  calcularMudanca(&Procs[0],0); // verifica as condições de ativação de cada processo (fiz um por um pela estática)
  calcularMudanca(&Procs[1],1);
  calcularMudanca(&Procs[2],2);
  calcularMudanca(&Procs[3],3);
  calcularMudanca(&Procs[4],4);
  calcularMudanca(&Procs[5],5);
}