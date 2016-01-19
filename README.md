# Bomberman

Projeto final da disciplina de Linguagens de Programação Estruturada - Ling. C

Objetivo
--------

Implementar uma estrutura de jogo básica, baseado no famoso bomberman.

Comandos
--------

- **Setas** Movimentam o personagem
- **Espaço** Planta a bomba
- **P** Pausa/retorna o jogo
- **ESC** Encerra a partida

Pontuação
---------

- **Caixas** Valem 1 ponto cada
- **Balões** Valem 10 pontos cada

## Compilação e dependências

Dependências
------------

- **Linux** depende do pacote de desenvolvimento ncurses
  - **RPM based** `ncurses-devel`
  - **Debian based** `libncurses-dev` ou `libncurses5-dev`

- **Windows** depende do pacote PDCurses.
 - [Tutorial de instalação PDCurses](http://www.codando.com/blog/?tag=pdcurses).
 
Compilação
----------

- **Linux** `gcc -Wall -Wextra -Wpedantic -std=c99 main.c -o Bomberman -lncurses`

- **Windows** `gcc -Wall -Wextra -pedantic -std=c99 main.c -o Bomberman.exe -lncurses`

## Autor

[Geancarlo Bernardes](https://github.com/bernardesGean)
