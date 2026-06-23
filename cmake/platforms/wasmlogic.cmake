#
# wasmlogic.cmake: Build headless logic-only WASM modules for puzzles.
#
# Selected when WASM_HEADLESS is enabled, regardless of compiler.
# This is used by online-puzzles (or any server-side caller) to load
# the pure back-end logic in Node and replay a player's movelog to
# verify a win.
#
# Unlike emscripten.cmake, this platform:
#  - does not include any front-end sources in core_obj,
#  - does not build any GUI puzzle executables,
#  - does not build CLI tools,
#  - exports a small, fixed set of C entry points per puzzle.
#
# Use a normal cmake configure but pass -DWASM_HEADLESS=ON.
# The compiler must be Emscripten (emcc) for the WASM output.
#

set(platform_common_sources "")
set(platform_gui_libs)
set(platform_libs)

set(WASM_HEADLESS_KEEP_FUNCS
  _main
  _malloc
  _free
  _logic_new_desc
  _logic_validate
  _logic_replay
  _logic_free_string
  _logic_default_params
  _midend_get_movelog
  _midend_which_game
  _midend_new
  _midend_free
  _midend_set_params
  _midend_new_game
  _midend_game_id
  _midend_solve
  _midend_status
  _midend_process_key
  _midend_text_format
  _midend_can_format_as_text_now)

list(TRANSFORM WASM_HEADLESS_KEEP_FUNCS PREPEND \")
list(TRANSFORM WASM_HEADLESS_KEEP_FUNCS APPEND \")
string(JOIN "," WASM_HEADLESS_KEEP_STRING ${WASM_HEADLESS_KEEP_FUNCS})

# Standalone-WASM build: no front-end, only a tiny export surface.
set(CMAKE_EXECUTABLE_SUFFIX ".js")

if(CMAKE_C_COMPILER_ID MATCHES "Clang" AND CMAKE_C_COMPILER MATCHES "emcc")
  set(CMAKE_C_LINK_FLAGS "\
-s ALLOW_MEMORY_GROWTH=1 \
-s ENVIRONMENT=node,web \
-s EXPORTED_FUNCTIONS='[${WASM_HEADLESS_KEEP_STRING}]' \
-s EXPORTED_RUNTIME_METHODS='[cwrap,UTF8ToString,stringToUTF8,lengthBytesUTF8]' \
-s MODULARIZE=1 \
-s EXPORT_NAME=Module \
-s STRICT_JS=1 \
-s STANDALONE_WASM=0 \
-s WASM=1")
else()
  message(FATAL_ERROR
    "WASM_HEADLESS=ON requires the Emscripten (emcc) compiler. "
    "Re-configure with 'emcmake cmake -B build-wasm -DWASM_HEADLESS=ON .'")
endif()

set(build_individual_puzzles FALSE)
set(build_cli_programs FALSE)
set(build_gui_programs FALSE)
set(build_icons FALSE)

function(get_platform_puzzle_extra_source_files OUTVAR NAME AUXILIARY)
  set(${OUTVAR} PARENT_SCOPE)
endfunction()

function(set_platform_puzzle_target_properties NAME TARGET)
endfunction()

function(set_platform_gui_target_properties TARGET)
endfunction()

function(build_platform_extras)
endfunction()
