### Secção: Análise de Otimização Estática e Supressão de Código (Arquitetura AVR)

Para validar o impacto mecânico do sistema de *erasure* e da herança estática na arquitetura AVR (ATmega328P), o fluxo principal de execução (`main`) foi compilado e analisado sob dois cenários distintos através do `avr-objdump`.

#### 1. Cenário: Funções Ativas (`with functions active.txt`)

Quando os componentes de depuração ou de transmissão linear estão ativos, o compilador mapeia as chamadas utilizando resolução estática direta, resolvendo os endereços em tempo de compilação sem depender de tabelas de métodos virtuais (`vtable`).

* 
**Acoplamento de Registos:** O endereço base do objeto `Print` é carregado diretamente no par de registos `r25:r24` (`0x011C`).


* 
**Invocações Lineares:** São geradas três chamadas consecutivas e explícitas ao método especializado de escrita:


```avr
580:	0e 94 91 01 	call	0x322	; Print::write(char const*)
58c:	0e 94 91 01 	call	0x322	; Print::write(char const*)
598:	0e 94 91 01 	call	0x322	; Print::write(char const*)
5a0:	0e 94 71 01 	call	0x2e2	; HardwareSerial::flush()

```


* 
**Ponteiros de Dados:** Os argumentos para cada string individual são passados via registos `r23:r22` antes de cada instrução `call`.



#### 2. Cenário: Funções Desativadas / Fallback (`functions deactivated.txt`)

Ao aplicar a técnica de supressão por fallback (onde as funções herdam de uma estrutura com métodos `constexpr` vazios), o compilador executa uma otimização de Eliminação de Código Morto (*Dead Code Elimination* - DCE).

* 
**Eliminação do Fluxo Repetitivo:** Toda a sequência de três chamadas dedicadas a `Print::write(char const*)` é completamente expurgada do corpo do `<main>`.


* 
**Inlining e Consolidação:** Em vez de preservar chamadas individuais redundantes ou vazias, o compilador limpa os registos e reduz a operação a uma única invocação de bloco genérico:


```avr
566:	0e 94 84 00 	call	0x108	; Print::write(unsigned char const*, unsigned int)
56e:	0e 94 71 01 	call	0x2e2	; HardwareSerial::flush()

```


* 
**Otimização de Espaço Flash:** A remoção das instruções repetitivas de carregamento de registos (`ldi`) e das sequências de `call` encolheu o tamanho do binário na Flash, deslocando o offset de retorno do benchmark de microssegundos de `0x5b8` para `0x586`.



---

### Matriz de Impacto Arquitetural

| Métrica Analisada | Funções Ativas (`active.txt`) | Funções Desativadas (`deactivated.txt`) |
| --- | --- | --- |
| **Resolução em Runtime** | Estática Direta (Sem `vptr`/`vtable`) 

 | Nula (Código Purgado) 

 |
| **Invocações de Escrita** | 3 chamadas a `Print::write(char const*)` 

 | 1 chamada genérica a `Print::write(...)` 

 |
| **Overhead de RAM** | 0 bytes adicionais | 0 bytes adicionais |
| **Ponteiros de String** | Carregados individualmente em registos 

 | Ignorados pelo fluxo principal de execução 

 |