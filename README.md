# openapi-downgrader  
A CLI tool that converts OpenAPI 3.0+ YAML to Swagger 2.0 format.  

### Usage  
```
openapi-downgrader.exe input.yaml output.yaml
```

Use [editor.swagger.io](https://editor.swagger.io) to verify the output. 

External references, both files and remote resources, are currently NOT supported - the input YAML must contain the entire spec.  

Loosely based on [api-spec-converter](https://github.com/LucyBot-Inc/api-spec-converter), but trimmed down and rewritten in C++ for speed and ease of use. 

Uses [yaml-cpp](https://github.com/jbeder/yaml-cpp).  
Built using Visual Studio 2022.

### Web Version

A browser-based version is [available](https://c6-dev.github.io/openapi-downgrader) that runs the conversion entirely client-side via WebAssembly (no server required). It can be hosted on GitHub Pages or any static file host.

See [`wasm/README.md`](wasm/README.md) for build instructions.
