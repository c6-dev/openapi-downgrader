#pragma once

#include <string>
#include "yaml-cpp/yaml.h"

class Converter {
public:
    std::string Convert(const std::string& source);
private:
    YAML::Node input;
	void ConvertInfos();
    YAML::Node ResolveReference(YAML::Node obj, bool shouldClone);
    void ConvertParameters(YAML::Node& obj);
    void ConvertOperations();
    void CopySchemaProperties(YAML::Node& node, std::vector<std::string> props);
    void CopySchemaXProperties(YAML::Node& node);
    void ConvertOperationParameters(YAML::Node& operation);
    void ConvertSchema(YAML::Node def, const std::string& operationDirection);
    void ConvertDiscriminatorMapping(YAML::Node mapping);
    void ConvertSecurityDefinitions();
    void ConvertResponses(YAML::Node& operation);
    void ConvertSchemas();
};
