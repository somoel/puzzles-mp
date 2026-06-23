/*
 * wasmlogic_main.c: entry point for the headless logic-only WASM
 * build of a single puzzle.
 *
 * Compiled together with <puzzle>.c and nullfe.c (plus core). Only
 * the four logic_* entry points listed below are exported; the rest
 * of the back-end is internal to the module.
 *
 * Build with -DSTANDALONE_WASM_LOGIC -DPUZZLE_NAME=<name>.
 *
 * Memory model: this module is a small library; the caller owns the
 * host process / WebAssembly instance and decides when to discard
 * it. Strings returned to the caller (logic_new_desc) are heap
 * allocations that the caller releases via logic_free_string.
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

static midend *logic_midend_new(void)
{
    /* The midend treats the frontend as an opaque pointer: it
     * stores it, hands it to callbacks, but never dereferences it
     * itself, and midend_free does not free it. We pass a
     * small sentinel solely so each midend owns a distinct
     * identity; nothing reads it. */
    static int sentinel;
    return midend_new((frontend *)&sentinel, &thegame, NULL);
}

static void logic_midend_free(midend *me)
{
    if (!me) return;
    midend_free(me);
}

/* ---- Public exports --------------------------------------------- */

EMSCRIPTEN_KEEPALIVE
const char *logic_new_desc(const char *params, const char *seed)
{
    midend *me = logic_midend_new();
    game_params *p = NULL;
    char *ret = NULL;
    const char *err;

    if (params && *params) {
        p = me->ourgame->default_params();
        me->ourgame->decode_params(p, params);
        err = me->ourgame->validate_params(p, true);
        if (err) {
            me->ourgame->free_params(p);
            midend_free(me);
            sfree(me->frontend);
            return NULL;
        }
        midend_set_params(me, p);
    }

    /* Midend_new_game expects a 15-digit seed, but we'll just pass
     * what we were given; if the caller supplies a usable seed,
     * the game is generated from it. */
    {
        char *prev_seed = me->seedstr;
        me->seedstr = seed ? dupstr(seed) : NULL;
        if (!me->seedstr) {
            /* fall back to a stable 15-digit string so callers can
             * still get something out of new_desc() with no seed */
            me->seedstr = dupstr("100000000000000");
        }
        midend_new_game(me);
        if (me->desc) {
            ret = dupstr(me->desc);
        }
        sfree(me->seedstr);
        me->seedstr = prev_seed;
    }

    logic_midend_free(me);
    return ret;
}

EMSCRIPTEN_KEEPALIVE
int logic_validate(const char *params, const char *desc)
{
    midend *me = logic_midend_new();
    char id[1024];
    const char *err;

    if (!desc) {
        logic_midend_free(me);
        return 0;
    }

    if (params && *params) {
        snprintf(id, sizeof(id), "%s:%s", params, desc);
    } else {
        snprintf(id, sizeof(id), ":%s", desc);
    }

    err = midend_game_id(me, id);
    logic_midend_free(me);
    return err == NULL;
}

EMSCRIPTEN_KEEPALIVE
int logic_replay(const char *params, const char *desc,
                 const char *const *moves, int nmoves)
{
    midend *me = logic_midend_new();
    char id[1024];
    const char *err;
    const game *g;
    game_state *state;
    int i, won;

    if (!desc) {
        logic_midend_free(me);
        return -1;
    }

    if (params && *params) {
        snprintf(id, sizeof(id), "%s:%s", params, desc);
    } else {
        snprintf(id, sizeof(id), ":%s", desc);
    }

    err = midend_game_id(me, id);
    if (err || me->statepos < 1) {
        logic_midend_free(me);
        return -1;
    }

    g = me->ourgame;
    state = g->dup_game(me->states[me->statepos - 1].state);

    for (i = 0; i < nmoves; i++) {
        game_state *next;
        if (!moves[i]) break;
        next = g->execute_move(state, moves[i]);
        if (!next) {
            g->free_game(state);
            logic_midend_free(me);
            return 0;
        }
        if (next != state) {
            g->free_game(state);
            state = next;
        }
    }

    won = (g->status(state) == 0);
    g->free_game(state);
    logic_midend_free(me);
    return won ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE
void logic_free_string(const char *s)
{
    sfree((void *)s);
}

/* Return the encoded-params string for the puzzle's default
 * parameters. Used by the host UI to send a sensible params
 * without enumerating presets in the browser. */
EMSCRIPTEN_KEEPALIVE
const char *logic_default_params(void)
{
    game_params *p = thegame.default_params();
    char *encoded = thegame.encode_params(p, false);
    thegame.free_params(p);
    /* encode_params returns a heap buffer; transfer ownership to
     * the caller. */
    return encoded;
}

/* Tiny main so emcc has an entry point. It is not normally
 * invoked; modules are consumed via cwrap from JS/Node. */
int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    return 0;
}
