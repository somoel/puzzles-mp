/*
 * wasmlogic_main.c: entry point for the headless logic-only WASM
 * build of a single puzzle.
 *
 * Compiled together with <puzzle>.c, nullfe.c and core. Only the
 * small set of logic_* entry points listed below are exported; the
 * rest of the back-end is internal to the module.
 *
 * Build with -DSTANDALONE_WASM_LOGIC -DPUZZLE_NAME=<name>.
 *
 * Memory model: this module is a small library; the caller owns
 * the host process / WebAssembly instance and decides when to
 * discard it. Strings returned to the caller (logic_new_desc) are
 * heap allocations that the caller releases via
 * logic_free_string.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#else
/* Allow non-Emscripten builds to compile the same file (e.g. for a
 * native smoke test of the logic_* entry points). The exports simply
 * become regular symbols. */
#define EMSCRIPTEN_KEEPALIVE
#endif

#include "puzzles.h"

#ifndef PUZZLE_NAME
#error "PUZZLE_NAME must be defined when building a logic WASM module"
#endif

extern const game thegame;

/* Make a game_params from a string, with the puzzle's default
 * params as the base. */
static game_params *params_from_string(const char *params)
{
    game_params *p = thegame.default_params();
    if (params && *params) {
        thegame.decode_params(p, params);
    }
    return p;
}

/* ---- Public exports --------------------------------------------- */

EMSCRIPTEN_KEEPALIVE
void logic_free_string(const char *s)
{
    sfree((void *)s);
}

EMSCRIPTEN_KEEPALIVE
const char *logic_default_params(void)
{
    game_params *p = thegame.default_params();
    char *encoded = thegame.encode_params(p, false);
    thegame.free_params(p);
    return encoded;
}

EMSCRIPTEN_KEEPALIVE
const char *logic_new_desc(const char *params, const char *seed)
{
    game_params *p;
    char *ret = NULL;
    char *aux = NULL;
    random_state *rs;
    char default_seed[16];

    if (!seed || !*seed) {
        memcpy(default_seed, "100000000000000", 15);
        default_seed[15] = '\0';
        seed = default_seed;
    }

    p = params_from_string(params);
    rs = random_new(seed, strlen(seed));
    ret = thegame.new_desc(p, rs, &aux, false);
    random_free(rs);
    sfree(aux);
    thegame.free_params(p);
    return ret;
}

EMSCRIPTEN_KEEPALIVE
const char *logic_get_game_id_error(const char *params, const char *desc)
{
    game_params *p = params_from_string(params);
    const char *err;
    char *ret;

    if (!desc) {
        thegame.free_params(p);
        return NULL;
    }
    err = thegame.validate_desc(p, desc);
    if (!err) {
        thegame.free_params(p);
        return NULL;
    }
    ret = dupstr(err);
    thegame.free_params(p);
    return ret;
}

EMSCRIPTEN_KEEPALIVE
int logic_validate(const char *params, const char *desc)
{
    const char *err = logic_get_game_id_error(params, desc);
    if (err) {
        logic_free_string(err);
        return 0;
    }
    return 1;
}

EMSCRIPTEN_KEEPALIVE
int logic_replay(const char *params, const char *desc,
                 const char *const *moves, int nmoves)
{
    game_params *p = params_from_string(params);
    game_state *state;
    int i, won;

    if (!desc) {
        thegame.free_params(p);
        return -1;
    }
    state = thegame.new_game(NULL, p, desc);
    thegame.free_params(p);
    if (!state) {
        return -1;
    }

    for (i = 0; i < nmoves; i++) {
        game_state *next;
        if (!moves[i]) break;
        next = thegame.execute_move(state, moves[i]);
        if (!next) {
            thegame.free_game(state);
            return 0;
        }
        if (next != state) {
            thegame.free_game(state);
            state = next;
        }
    }

    won = (thegame.status(state) > 0);
    thegame.free_game(state);
    return won ? 1 : 0;
}

/* Tiny main so emcc has an entry point. It is not normally
 * invoked; modules are consumed via cwrap from JS/Node. */
int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    return 0;
}
