# Vendored flatcc runtime (C FlatBuffers)

- `include/flatcc/**` + `runtime/{builder,emitter,refmap}.c` — flatcc runtime
  (https://github.com/dvidelabs/flatcc), the minimal set to BUILD FlatBuffers
  (reading is header-only). Verifier/JSON runtime not vendored.
- `bridge_generated.h` — generated from `bridge.fbs` by `flatcc -a`.
- `bridge.fbs` — the wire schema (source of truth; the Python side is generated from
  the same file by `flatc --python`, vendored in Bot_Python_IA/src/UrtBridge).

Regenerate after a schema change:
    flatcc -a --outfile bridge_generated.h bridge.fbs
