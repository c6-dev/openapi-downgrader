# openapi-downgrader  
A CLI tool that converts OpenAPI 3.0 YAML to Swagger 2.0 format.  

### Usage  
```
openapi-downgrader.exe input.yaml output.yaml
```

Use [editor.swagger.io](https://editor.swagger.io) to verify the output. 

Loosely based on [api-spec-converter](https://github.com/LucyBot-Inc/api-spec-converter), but trimmed down and rewritten in C++ for speed and ease of use. 

Uses [yaml-cpp](https://github.com/jbeder/yaml-cpp).  
Built using Visual Studio 2022.
