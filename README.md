# openapi-downgrader  
Downgrades a YAML API spec from OpenAPI 3.0 to Swagger 2.0 format.  
Made for personal use and hasn't been thoroughly tested. Something like [editor.swagger.io](https://editor.swagger.io) should always be used to verify the output. 

Usage: `openapi-downgrader.exe input.yaml output.yaml`  

Loosely based on [api-spec-converter](https://github.com/LucyBot-Inc/api-spec-converter), but trimmed down and rewritten in C++ for speed and ease of use. 

Uses [yaml-cpp](https://github.com/jbeder/yaml-cpp).  
Built using Visual Studio 2022.
