# Atividade 3 — Sistemas Embarcados

Projetos para a Black Pill **STM32F411CEU6**, com gravação e comunicação pelo
USB da própria placa. Requisitos: [Enunciado.pdf](Enunciado.pdf).

| Questão | Projeto | Situação |
| --- | --- | --- |
| (a) | [Contador de pressionamentos](atividade3/questao_a/README.md) | Implementado e validado |
| (b) | [Tempo de reação](atividade3/questao_b/README.md) | Implementado e validado |
| (c) | [Decodificador Morse](atividade3/questao_c/README.md) | Etapa 2: lógica testada; integração pendente |
| (d) | [Sequência rítmica](atividade3/questao_d/README.md) | A implementar |

Cada questão terá seus próprios fontes, testes e documentação. A questão (a)
já contém `src/`, `inc/`, `tests/`, `docs/` e `CMakeLists.txt`. A questão (b)
tem firmware independente, testes e evidências de validação. A questão (c)
tem sua base compilável. A pasta (d) contém o escopo e ainda não gera firmware.

## Estrutura compartilhada

- `plataforma/stm32f411/`: toolchain ARM GCC, mapa de memória e função CMake
  que integra os fontes da questão ao startup, CMSIS, HAL e USB CDC.
- [vendor/](vendor/README.md): dependências com versões, origens e licenças registradas.
- `scripts/`: compilação por questão, gravação DFU e monitor serial.
- `out/build/questao_a/`, `questao_b/` e `questao_c/`: artefatos separados por
  projeto; a próxima usará `questao_d/`. Todo `out/` é ignorado pelo Git.
- [template/](template/README.md): arquivos antigos preservados.

Os adaptadores USB, clock, SysTick e lógica da aplicação ficam nos fontes
de cada questão. A base de (b) foi copiada de (a), sem dependência entre elas.
As bibliotecas da ST e a infraestrutura CMake continuam compartilhadas.
Cada projeto terá seu próprio `main` e será gravado separadamente na placa;
gravar uma questão substitui o firmware anterior.

## Compilar e gravar

Abra a raiz **ELEW32** no VS Code. ARM GNU Toolchain, CMake e Ninja devem estar
no PATH. Use as extensões C/C++ e CMake Tools recomendadas pelo workspace.
O [histórico](atividade3/questao_a/docs/historico.md) registra a instalação e
as versões utilizadas. Os comandos abaixo são executados a partir da raiz:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/project.ps1 -Question a -Action Build
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/test-button.ps1
```

O build gera `questao_a.elf`, `.hex`, `.bin` e `.map` em `out/build/questao_a/`.
Os testes C da questão (a) geram seu executável em `out/tests/questao_a/`.
Na primeira execução dos testes, acrescente `-Setup` para instalar o TCC local.

No VS Code, **Ctrl+Shift+B** e as tarefas **CMake: Configure**, **CMake: Clean**
e **DFU: Flash and Verify** pedem a questão. Se ela ainda não tiver firmware,
o comando informa isso e termina. O painel CMake Tools e o IntelliSense estão
configurados para (c), projeto em desenvolvimento; a seleção das tarefas não
altera a seleção desse painel. Para usar o painel com (a) ou (b), ajuste os caminhos
`cmake.sourceDirectory` e `cmake.buildDirectory` em `.vscode/settings.json`.
Os exemplos abaixo continuam explícitos para (a); use `-Question b` para (b)
ou `-Question c` para (c).

Para preparar o dfu-util uma vez, listar a placa em DFU e compilar/gravar (a):

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/setup-dfu.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/dfu.ps1 -Action List
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/project.ps1 -Question a -Action Flash
```

Entre em DFU segurando BOOT0, apertando e soltando RESET e soltando BOOT0 após
meio segundo. O dispositivo **STM32 BOOTLOADER (0483:DF11)** usa WinUSB.
Após a gravação e verificação, pressione RESET com BOOT0 solto. Consulte o
[guia da questão (a)](atividade3/questao_a/docs/questao-a.md) para detalhes.
O comando direto `scripts/dfu.ps1 -Action Flash -Question a` grava o BIN já
compilado; a tarefa do VS Code e `project.ps1 -Action Flash` compilam antes.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/serial-monitor.ps1
```

O monitor procura a porta USB CDC automaticamente e ativa DTR. Não reutilize
os caches antigos de `template/build/` ou `out/build/windows/`; eles permanecem
preservados como artefatos anteriores à reorganização.
