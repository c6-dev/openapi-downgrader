#include "../code/Converter.h"
#include <emscripten/bind.h>
#include <stdexcept>

std::string convert(std::string yamlInput) {
    Converter converter;
    return converter.ConvertString(yamlInput);
}

EMSCRIPTEN_BINDINGS(openapi_downgrader) {
    emscripten::function("convert", &convert);
}
