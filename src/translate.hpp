#pragma once
#include <string>
#include <map>
#include <memory>
#define INDEBUG 1
typedef std::map<
    std::string,
    std::pair<
        std::string,
        std::string
    >
> generic_classes_t;

typedef std::map<
    std::string,
    std::string
> transpile_time_variables_t;

typedef std::map<
    std::string,
    std::string
> tasks_t;

typedef struct {
    std::unique_ptr<generic_classes_t> generics = std::make_unique<generic_classes_t>();
    std::unique_ptr<transpile_time_variables_t> variables = std::make_unique<transpile_time_variables_t>();
    std::unique_ptr<tasks_t> tasks = std::make_unique<tasks_t>();
} info_t;
std::string translate(const std::string &, info_t *info);