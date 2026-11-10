#!/bin/sh
# Compile le jeu pour PC avec le stub libnds (tests uniquement)
cd "$(dirname "$0")/.." || exit 1
mkdir -p host/out
gcc -std=gnu99 -O1 -g -Wall -Wextra -Wno-unused-parameter -fsanitize=address,undefined -Ihost -Iinclude \
    source/*.c host/host.c host/bot.c -o host/out/game_bot "$@"
