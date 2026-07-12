# BrainBoost — Treinamento Cognitivo

![Dashboard do BrainBoost](assets/img/dashboard.png)
Aplicativo desktop de treinamento cerebral escrito em **C++17**, com interface
gráfica **100% própria** (nenhuma biblioteca de UI de terceiros no repositório):
o desenho é feito direto com **SDL2** e o texto é renderizado com **FreeType**,
ambas bibliotecas do sistema.

## Como compilar e executar

Dependências (todas de sistema): `cmake`, `g++`, SDL2 (dev) e FreeType (dev).

```sh
cmake -B build
cmake --build build -j$(nproc)
./run.sh
```

> **Nota (Flatpak/VSCode):** prefira `./run.sh` a chamar o binário direto. O
> terminal do VSCode Flatpak injeta bibliotecas do host via `LD_LIBRARY_PATH`
> e isso pode atrapalhar a criação da janela; o script remove a variável antes
> de executar. (Há também um fallback para renderização por software.)

## Funcionalidades

- **Dashboard** com cards de jogos, indicadores de XP, nível e sequência de dias,
  barras de desempenho por categoria e gráfico de evolução das sessões.
- **4 jogos jogáveis**: Memória Numérica, Cálculo Mental, Sequência Lógica e
  Reação Rápida (os demais cards aparecem como "Em breve").
- **Progressão**: XP, níveis (500 XP por nível) e sequência diária (streak).
- **Conquistas** desbloqueáveis com recompensa em XP.
- **Salvamento local automático** em `brainboost_save.ini` (formato texto
  `chave=valor`), gravado ao fim de cada jogo e ao fechar o app.

## Estrutura do projeto

Headers ficam em `include/` e implementações em `src/`, espelhando as mesmas
subpastas:

```
include/            # todos os .h
├── app/            # Application (janela+loop), AppContext (estado compartilhado)
├── core/           # regras de negócio, sem nenhum código de UI
│   ├── GameCategory.h   # categorias cognitivas
│   ├── GameInfo.h       # metadados + factory de cada jogo
│   ├── GameRegistry.h   # catálogo de jogos (ponto único de registro)
│   ├── GameResult.h     # resultado de uma sessão
│   ├── UserProfile.h    # nome, XP, nível, streak, conquistas
│   ├── Statistics.h     # habilidade por categoria + histórico
│   ├── Achievements.h   # definições e condições de desbloqueio
│   └── SaveManager.h    # persistência local chave=valor
├── games/          # interface Game + um header por jogo
└── ui/             # interface gráfica própria
    ├── Theme.h          # struct Color + paleta central
    ├── Rect.h           # retângulo (layout e hit-test)
    ├── Renderer.h       # desenho 2D + texto TTF via FreeType (cache de glifos)
    ├── Input.h          # snapshot de mouse/teclado por frame
    ├── Widgets.h        # botão, card, barras, chip, gráfico, campo de texto
    ├── ScreenId.h       # enum das páginas
    └── *Screen.h        # Sidebar, Home, Game, Stats, Achievements, Settings, About

src/                # todos os .cpp (mesma organização)
└── main.cpp        # ponto de entrada
```

**Fluxo de dados:** as telas recebem `AppContext&` (dados) + `Renderer&`/
`Input&` (UI) e nunca tocam SDL diretamente; a camada `core` não conhece nada
de interface. Cores vivem apenas em `Theme.h`.

## Como adicionar um novo jogo

1. Crie `include/games/MeuJogo.h` e `src/games/MeuJogo.cpp` implementando a
   interface `Game` (`frame`, `isFinished`, `result`) — use um jogo existente
   como modelo.
2. Registre-o em `src/core/GameRegistry.cpp` com título, descrição, categoria,
   cor do card e `makeFactory<MeuJogo>()`.
3. Adicione o `.cpp` ao `CMakeLists.txt`.

Cards, XP, estatísticas, gráfico e conquistas passam a considerá-lo
automaticamente.
# BrainBoost
