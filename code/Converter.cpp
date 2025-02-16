#include "converter.h"
#include "Util.h"
#include <iostream>
#include <regex>
#include "yaml-cpp/yaml.h"

const std::array<std::string, 8> http_methods = {"get", "put", "post", "delete", "options", "head", "patch", "trace"};
const std::vector<std::string> schema_properties = { "format", "minimum", "maximum", "exclusiveMinimum", "exclusiveMaximum", "minLength", "maxLength", "multipleOf", "minItems", "maxItems", "uniqueItems", "minProperties", "maxProperties", "additionalProperties", "pattern", "enum", "default" };
const std::vector<std::string> array_properties = { "type", "items" };


YAML::Node Converter::ResolveReference(YAML::Node obj, bool shouldClone) {

	if (!obj || !obj["$ref"]) return obj;
    auto ref = obj["$ref"].as<std::string>();
    if (ref[0]=='#') {
        std::vector<std::string> keys = util::SplitAndDecode(ref);
        YAML::Node cur = obj;
        YAML::Node result = util::Navigate(cur, keys);

        return shouldClone ? YAML::Clone(result) : result;
    }
    return obj;
}
void Converter::ConvertInfos() {
    if (input["servers"] && input["servers"][0]) {
        YAML::Node server = input["servers"][0];
        std::string serverUrl = server["url"].as<std::string>();
        YAML::Node variables = server["variables"];

        if (variables && variables.IsMap()) {
            for (auto it = variables.begin(); it != variables.end(); ++it) {
                std::string variableName = it->first.as<std::string>();
                YAML::Node variableObject = it->second;

                if (variableObject["default"]) {
                    std::string defaultValue = variableObject["default"].as<std::string>();

                    std::string placeholder = "{" + variableName + "}";
                    size_t pos = 0;
                    while ((pos = serverUrl.find(placeholder, pos)) != std::string::npos) {
                        serverUrl.replace(pos, placeholder.length(), defaultValue);
                        pos += defaultValue.length();
                    }
                }
            }
        }
        util::URL parsed = util::ParseURL(serverUrl);
		if (parsed.host.empty()) {
			input.remove("host");
		}
		else {
			input["host"] = parsed.host;
		}
        if (parsed.protocol.empty()) {
			input.remove("schemes");
		}
        else {
            input["schemes"] = YAML::Node(YAML::NodeType::Sequence);
            input["schemes"].push_back(parsed.protocol);
        }
		input["basePath"] = parsed.path;
    }
	input.remove("servers");
    input.remove("openapi");
}

void Converter::ConvertDiscriminatorMapping(YAML::Node mapping) {
    for (auto it = mapping.begin(); it != mapping.end(); ++it) {
        std::string payload = it->first.as<std::string>();
        std::string schemaNameOrRef = it->second.as<std::string>();

        if (schemaNameOrRef.empty()) {
            std::cerr << "Ignoring " << schemaNameOrRef << " for " << payload << " in discriminator.mapping." << std::endl;
            continue;
        }

        YAML::Node schema;
        std::regex schemaNameRegex(R"(^[a-zA-Z0-9._-]+$)");
        if (std::regex_match(schemaNameOrRef, schemaNameRegex)) {
            try {
                schema = ResolveReference(YAML::Node(YAML::NodeType::Map)["$ref"] = "#/components/schemas/" + schemaNameOrRef, false);
            }
            catch (const std::exception& err) {
                std::cerr << "Error resolving " << schemaNameOrRef << " for " << payload << " as schema name in discriminator.mapping: " << err.what() << std::endl;
            }
        }

        if (!schema) {
            try {
                schema = ResolveReference(YAML::Node(YAML::NodeType::Map)["$ref"] = schemaNameOrRef, false);
            }
            catch (const std::exception& err) {
                std::cerr << "Error resolving " << schemaNameOrRef << " for " << payload << " in discriminator.mapping: " << err.what() << std::endl;
            }
        }

        if (schema) {
            schema["x-discriminator-value"] = payload;
            schema["x-ms-discriminator-value"] = payload;
        }
        else {
            std::cerr << "Unable to resolve " << schemaNameOrRef << " for " << payload << " in discriminator.mapping." << std::endl;
        }
    }
}

void Converter::ConvertSchema(YAML::Node def, const std::string& operationDirection) {
    if (def["oneOf"]) {
        def.remove("oneOf");
        if (def["discriminator"]) {
            def.remove("discriminator");
        }
    }

    if (def["anyOf"]) {
        def.remove("anyOf");
        if (def["discriminator"]) {
            def.remove("discriminator");
        }
    }

    if (def["allOf"]) {
        for (auto item : def["allOf"]) {
            ConvertSchema(item, operationDirection);
        }
    }

    if (def["discriminator"]) {
        if (def["discriminator"]["mapping"]) {
            ConvertDiscriminatorMapping(def["discriminator"]["mapping"]);
        }
        def["discriminator"] = def["discriminator"]["propertyName"];
    }

    std::string type = def["type"].as<std::string>("");
    if (type == "object") {
        if (def["properties"]) {
            for (auto it = def["properties"].begin(); it != def["properties"].end(); ++it) {
                std::string propName = it->first.as<std::string>();
                YAML::Node prop = it->second;
                if (prop["writeOnly"] && prop["writeOnly"].as<bool>() == true && operationDirection == "response") {
                    def["properties"].remove(propName);
                }
                else {
                    ConvertSchema(prop, operationDirection);
                    prop.remove("writeOnly");
                }
            }
        }
    }
    else if (type == "array") {
        if (def["items"]) {
            ConvertSchema(def["items"], operationDirection);
        }
    }

    if (def["nullable"]) {
        def["x-nullable"] = true;
        def.remove("nullable");
    }

    if (def["deprecated"]) {
        if (!def["x-deprecated"]) {
            def["x-deprecated"] = def["deprecated"];
        }
        def.remove("deprecated");
    }
}


void Converter::ConvertOperationParameters(YAML::Node& operation) {
    YAML::Node content, param;
    std::string contentKey;
    std::vector<std::string> mediaRanges, mediaTypes;

    if (!operation["parameters"]) {
        operation["parameters"] = YAML::Node(YAML::NodeType::Sequence);
    }

    if (operation["requestBody"]) {
        param = ResolveReference(operation["requestBody"], true);

        // Fixing external $ref in body
        if (operation["requestBody"]["content"]) {
            auto supportedMimeTypes = util::GetSupportedMimeTypes(operation["requestBody"]["content"]);
            if (!supportedMimeTypes.empty()) {
                std::string type = supportedMimeTypes[0];
                YAML::Node structuredObj = YAML::Node(YAML::NodeType::Map);
                structuredObj["content"] = YAML::Node(YAML::NodeType::Map);
                YAML::Node data = operation["requestBody"]["content"][type];

                if (data && data["schema"] && data["schema"]["$ref"] && data["schema"]["$ref"].as<std::string>()[0] != '#') {
                    std::cerr << "external refs aren't supported" << std::endl;
                }
            }
        }

        param["name"] = "body";
        content = param["content"];
        if (content && content.size() > 0) {
            mediaRanges = util::GetMediaRanges(content);
            mediaTypes = util::GetMediaTypes(mediaRanges);
            auto supportedMimeTypes = util::GetSupportedMimeTypes(content);
            if (!supportedMimeTypes.empty()) {
                contentKey = supportedMimeTypes[0];
                param.remove("content");

                if (contentKey == "application/x-www-form-urlencoded" || contentKey == "multipart/form-data") {
                    operation["consumes"] = mediaTypes;
                    param["in"] = "formData";
                    param["schema"] = content[contentKey]["schema"];
                    param["schema"] = ResolveReference(content[contentKey]["schema"], true);
                    if (param["schema"]["type"].as<std::string>() == "object" && param["schema"]["properties"]) {
                        auto required = param["schema"]["required"];
                        for (const auto& name : param["schema"]["properties"]) {
                            std::string varName = name.first.as<std::string>();
                            YAML::Node schema = param["schema"]["properties"][varName];
                            if (!schema["readOnly"]) {
                                YAML::Node formDataParam;
                                formDataParam["name"] = varName;
                                formDataParam["in"] = "formData";
                                formDataParam["schema"] = schema;
                                for (const auto& reqProp : required) {
                                    if (reqProp.as<std::string>() == varName) {
                                        formDataParam["required"] = true;
                                        break;
                                    }
                                }
                                operation["parameters"].push_back(formDataParam);
                            }
                        }
                    }
                    else {
                        operation["parameters"].push_back(param);
                    }
                }
                else if (!contentKey.empty()) {
                    operation["consumes"] = mediaTypes;
                    param["in"] = "body";
                    param["schema"] = content[contentKey]["schema"];
                    operation["parameters"].push_back(param);
                }
                else if (!mediaRanges.empty()) {
                    operation["consumes"] = !mediaTypes.empty() ? mediaTypes : std::vector<std::string>{ "application/octet-stream" };
                    param["in"] = "body";
                    param["name"] = param["name"] ? param["name"].as<std::string>() : "file";
                    param.remove("type");
                    if (content[mediaRanges[0]]["schema"]) {
                        param["schema"] = content[mediaRanges[0]]["schema"];
                    }
                    else {
                        param["schema"]["type"] = "string";
                        param["schema"]["format"] = "binary";
                    }
                    operation["parameters"].push_back(param);
                }

                if (param["schema"]) {
                    ConvertSchema(param["schema"], "request");
                }
            }
        }
        operation.remove("requestBody");
    }
    ConvertParameters(operation);
}

void Converter::ConvertResponses(YAML::Node& operation) {

    for (auto it = operation["responses"].begin(); it != operation["responses"].end(); ++it) {
        auto code = it->first.as<std::string>();
        auto response = ResolveReference(it->second, true);
        if (response["content"]) {
            auto anySchema = YAML::Node();
            auto jsonSchema = YAML::Node();

            for (auto contentIt = response["content"].begin(); contentIt != response["content"].end(); ++contentIt) {
                auto mediaRange = contentIt->first.as<std::string>();
                auto mediaType = (mediaRange.find('*') == std::string::npos) ? mediaRange : "application/octet-stream";
                auto produces = operation["produces"];
                if (!operation["produces"]) {
                    operation["produces"] = YAML::Node(YAML::NodeType::Sequence);
                    operation["produces"].push_back(mediaType);
                }
                else {
                    bool found = false;
                    for (const auto& existing : operation["produces"]) {
                        if (existing.as<std::string>() == mediaType) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        operation["produces"].push_back(mediaType);
                    }
                }

                auto content = response["content"][mediaRange];

                if (anySchema.IsNull() && content["schema"]) {
                    anySchema = content["schema"];
                }
                if (jsonSchema.IsNull() && util::IsJsonMimeType(mediaType) && content["schema"]) {
                    jsonSchema = content["schema"];
                }

                if (content["example"]) {
                    if (!response["examples"]) {
                        response["examples"] = YAML::Node(YAML::NodeType::Map);
                    }
                    response["examples"][mediaType] = content["example"];
                }
            }

            if (anySchema) {
                response["schema"] = jsonSchema.IsNull() == false ? jsonSchema : anySchema;

                ConvertSchema(response["schema"], "response");
            }
        }

        auto headers = response["headers"];
        if (headers) {
            for (auto headerIt = headers.begin(); headerIt != headers.end(); ++headerIt) {
                std::string header = headerIt->first.as<std::string>();
                auto resolved = ResolveReference(headerIt->second, true);

                if (resolved["schema"]) {
                    resolved["type"] = resolved["schema"]["type"];
                    resolved["format"] = resolved["schema"]["format"];
                    resolved.remove("schema");
                }
                headers[header] = resolved;
            }
        }
        response.remove("content");
        operation["responses"][code] = response;
        
    }
}

void Converter::ConvertOperations(){
    YAML::Node paths = input["paths"];
    for (auto path = paths.begin(); path != paths.end(); ++path) {
        YAML::Node pathObject = ResolveReference(path->second, true);

        
        ConvertParameters(pathObject);
        for (auto method = pathObject.begin(); method != pathObject.end(); ++method) {
            std::string methodName = method->first.as<std::string>();
            auto it = std::find(http_methods.begin(), http_methods.end(), methodName);
            if (it != http_methods.end()) {
                auto operation = ResolveReference(method->second, true);
                ConvertOperationParameters(operation);
                ConvertResponses(operation);
            }
        }
    }
}

void Converter::CopySchemaProperties (YAML::Node& node, std::vector<std::string> props) {
    auto schema = ResolveReference(node["schema"], true);
    if (!schema) return;
    for (auto it : props)
    {
        auto value = schema[it.data()];
        if (value)
        {
            node[it.data()] = value;
        }
    }
} 

void Converter::CopySchemaXProperties(YAML::Node& node)
{
    auto schema = ResolveReference(node["schema"], true);
    for (const auto &kv: schema) {
        auto propName = kv.first.as<std::string>();
        if (propName.rfind("x-", 0) == 0 && schema[propName] && !node[propName])
            node[propName] = kv.second;
        }
} 

void Converter::ConvertParameters(YAML::Node& obj)
{ 
    if (!obj["parameters"]) return;
    auto params = obj["parameters"];
    for (const auto& item : params) {
        YAML::Node param = ResolveReference(item, false);
        
        std::string in = param["in"].as<std::string>();
        if (in != "body") {
            CopySchemaProperties(param, schema_properties);
            CopySchemaProperties(param, array_properties);
            CopySchemaXProperties(param);
            if (!param["description"])
            {
                auto schema = ResolveReference(param["schema"], false);
                if (schema && schema["description"])
                {
                    param["description"] = schema["description"];
                }
            }
            param.remove("schema");
            param.remove("allowReserved");
            if (param["example"])
            {
                param["x-example"] = param["example"];
                param.remove("example");
            }
        }
        if (param["type"] && param["type"].as<std::string>() == "array")
        {
            std::string style;

            if (param["style"])
            {
                style = param["style"].as<std::string>();
            } else if (in == "query" || in == "cookie")
	        {
                style = "form";
	        } else
	        {
                style = "simple";
	        }
            std::string explode = param["explode"].as<std::string>();
            if (style == "matrix") {
                if (param["explode"]) param["collectionFormat"] = "csv";
            }
            else if (style == "simple") {
                param["collectionFormat"] = "csv";
            }
            else if (style == "spaceDelimited") {
                param["collectionFormat"] = "ssv";
            }
            else if (style == "pipeDelimited") {
                param["collectionFormat"] = "pipes";
            }
            else if (style == "pipeObject") {
                param["collectionFormat"] = "multi";
            }
            else if (style == "form") {
                param["collectionFormat"] = (param["explode"].as<std::string>() == "false") ? "csv" : "multi";
            }
        }
        param.remove("style");
        param.remove("explode");
    }
}

void Converter::ConvertSchemas() {
    input["definitions"] = input["components"]["schemas"];

    for (auto it = input["definitions"].begin(); it != input["definitions"].end(); ++it) {
        ConvertSchema(it->second, "");
    }

    input["components"].remove("schemas");
}

void Converter::ConvertSecurityDefinitions() {
    input["securityDefinitions"] = input["components"]["securitySchemes"];

    for (auto it = input["securityDefinitions"].begin(); it != input["securityDefinitions"].end(); ++it) {
        YAML::Node security = it->second;
        std::string type = security["type"].as<std::string>();
        std::string scheme = security["scheme"].as<std::string>("");

        if (type == "http" && scheme == "basic") {
            security["type"] = "basic";
            security.remove("scheme");
        }
        else if (type == "http" && scheme == "bearer") {
            security["type"] = "apiKey";
            security["name"] = "Authorization";
            security["in"] = "header";
            security.remove("scheme");
            security.remove("bearerFormat");
        }
        else if (type == "oauth2") {
            auto flows = security["flows"];
            if (flows) {
                auto flowIt = flows.begin();
                std::string flowName = flowIt->first.as<std::string>();
                YAML::Node flow = flowIt->second;

                if (flowName == "clientCredentials") {
                    security["flow"] = "application";
                }
                else if (flowName == "authorizationCode") {
                    security["flow"] = "accessCode";
                }
                else {
                    security["flow"] = flowName;
                }

                security["authorizationUrl"] = flow["authorizationUrl"];
                security["tokenUrl"] = flow["tokenUrl"];
                security["scopes"] = flow["scopes"];
                security.remove("flows");
            }
        }
    }

    input["components"].remove("securitySchemes");
}

std::string Converter::Convert(const std::string& source) {
    input = YAML::LoadFile(source);;
    ConvertInfos();
    ConvertOperations();
    if (input["components"]) {
        ConvertSchemas();
        ConvertSecurityDefinitions();

        input["x-components"] = input["components"];
        input.remove("components");

        util::FixRefs(input);
    }
    YAML::Node result;
    result["info"] = input["info"];
    result["host"] = input["host"];
    result["basePath"] = input["basePath"];
    result["schemes"] = input["schemes"];
    result["paths"] = input["paths"];
    result["definitions"] = input["definitions"];
    result["securityDefinitions"] = input["securityDefinitions"];
    result["x-components"] = input["x-components"];
    YAML::Emitter out;
    out << result;
    std::string str ="swagger: \"2.0\"\n";
    str += out.c_str();
    return str;
}