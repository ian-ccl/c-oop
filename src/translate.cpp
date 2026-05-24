#include "translate.hpp"
#include <cstring>
#include <cctype>
#include <cstddef>
#include <algorithm>
#include <iostream>
#define INDEBUG 1

struct color_t {
    static constexpr const char* yellow = "\033[33m";
    static constexpr const char* red    = "\033[31m";
    const char* s = color_t::red;
    const char* const get() {
        if (s == color_t::red)
            s = color_t::yellow;
        else
            s = color_t::red;
        return s;
    }
} color;

#if INDEBUG
    #define log(x) do { std::cout << color.get() << x << "\033[0m"; } while(0)
#else
    #define log(x)
#endif

struct pragma_t {
    std::string name{""};
    bool active{false};
};
template<typename T, typename U>
std::ostream& operator<<(std::ostream& os, std::pair<T, U> pair) {
    os << '{' << pair.first << ", " << pair.second << '}';
    return os;
}

template<typename T, typename U>
std::ostream& operator<<(std::ostream& os, std::map<T, U> map) {
    for (const auto& [key, value] : map) {
        os << key << ": " << value << '\n';
    }
    return os;
}

bool isIdent(const char* const str, size_t i) {
    auto isFirst = [](unsigned char c) {
        return isalpha(c) || c == '_';
    };

    auto isRest = [](unsigned char c) {
        return isalnum(c) || c == '_';
    };

    if (!isRest(static_cast<unsigned char>(str[i]))) {
        return false;
    }

    size_t begin = i;
    while (begin > 0 && isRest(static_cast<unsigned char>(str[begin - 1]))) {
        --begin;
    }

    if (!isFirst(static_cast<unsigned char>(str[begin]))) {
        return false;
    }

    std::size_t end = i;
    while (str[end] != '\0' && isRest(static_cast<unsigned char>(str[end]))) {
        ++end;
    }
    return true;
}

bool isWord(const char* str, size_t i, const std::string& word) {
    size_t len = word.size();

    if (std::string(str + i, len) != word)
        return false;

    if (i > 0 && isIdent(str, i - 1))
        return false;

    if (str[i + len] != '\0' && isIdent(str, i + len))
        return false;

    return true;
}

bool scaped(const char* const str, size_t i) {
    if (i == 0) return false;
    size_t k = 0;
    for (size_t j = i;  j > 0 && str[j-1] == '\\'; j--) {
        k++;
    }
    return k % 2 == 1;
}

std::string replace(const std::string_view &src, const char*const pattern, const char*const replace) {
    std::string res;
    size_t n = src.size();
    size_t m = strlen(pattern);
    size_t i = 0;
    while (i < n) {
        if (src.substr(i, m) == pattern) {
            res += replace;
            i+=m;
            continue;
        }
        res += src[i++];
    }
    return res;
}

std::string trim_init(const std::string& s) {
    size_t i = 0;
    for(; i < s.size() && std::isspace((unsigned char)s[i]); i++) {}
    return s.substr(i);
}

std::string trim_end(const std::string& s) {
    if (s.empty()) return s;
    auto s2 = s+' ';
    size_t i = s2.size()-1;
    for(; i > 0 && std::isspace((unsigned char)s2[i - 1]); i--) {}
    return s2.substr(0,i);
}

std::string trim(const std::string& s) {
    return trim_init(trim_end(s));
}

std::string translate_first(const std::string &coopCode, generic_classes_t *generics, transpile_time_variables_t *variables, tasks_t *tasks) {
    std::string res{}, actualNamespace{};
    size_t i = 0, n = coopCode.size();
    bool inStr = false, inChar = false, inComment = false, inBlockComment = false;
    while (i < n) {
        char c = coopCode[i];
        if (c == '"' && ! scaped(coopCode.c_str(), i) && ! inChar && !inComment && !inBlockComment) {
            inStr = !inStr;
        }

        if (c == '\'' && ! scaped(coopCode.c_str(), i) && ! inStr && !inComment && !inBlockComment) {
            inChar = !inChar;
        }

        if (c == '/' && i+1 < n && coopCode[i+1] == '/' && !inChar && ! inStr && !inComment && !inBlockComment) {
            inComment = true;
        }

        if (c == '/' && i+1 < n && coopCode[i+1] == '*' && !inChar && ! inStr && !inComment && !inBlockComment) {
            inBlockComment = true;
        }

        if (c == '\n' && inComment) {
            inComment = false;
            for (i; i < n && isspace((unsigned char)coopCode[i]); i++) {}
            continue;
        }

        if (c == '*' && inBlockComment && i+1 < n && coopCode[i+1] == '/') {
            inBlockComment = false;

            i+= 2;
            for (i; i < n && isspace((unsigned char)coopCode[i]); i++) {}
            continue;
        }
        if (!(inStr || inChar || inComment || inBlockComment)) {
            if (isWord(coopCode.c_str(), i, "class")) {
                log("found class\n");
                res += "typedef struct ";
                i += 6;
                std::string name;
                for (;i < n && coopCode[i] != '{'; i++) {
                    name += coopCode[i];
                }
                name = replace(name, "::", "_");
                
                name = trim(name);

                res += name + " " + name +"; struct " + name;
                std::string structdef;
                for (;i < n && coopCode[i-1] != '}'; i++) {
                    structdef += coopCode[i];
                }

                for (i; i < n && isspace((unsigned char)coopCode[i]); i++) {}
                if (i < n && coopCode[i] == ';') {
                    structdef += ';';
                } else {
                    std::cerr << "warning: expected ';' after class definition of class '" << name << "'\n";
                }
                i++;

                for (i; i < n && isspace((unsigned char)coopCode[i]); i++) {}
                
                res += trim(replace(structdef, "::", "_"));
                log("class name: '" << name << "'\n");
                for (i; i < n && isspace((unsigned char)coopCode[i]); i++) {}
                continue;
            }
            if (coopCode.substr(i, 2) == "::") {
                res += "_";
                i += 2;
                for (i; i < n && isspace((unsigned char)coopCode[i]); i++) {}
                continue;
            }
            if (c == ':' && !(i > 0 && coopCode[i-1] == ':') && !(i+1 < n && coopCode[i+1] == ':')) {
                int j = i-1;
                std::string name;
                while (j >= 0 && (isalnum((unsigned char)coopCode[j]) || coopCode[j] == '_')) {
                    name += coopCode[j];
                    j--;
                }
                std::reverse(name.begin(), name.end());
                std::string method;
                i++;
                if (i < n && (isalpha((unsigned char)coopCode[i]) || coopCode[i] == '_')) {
                    method += coopCode[i];
                    i++;
                } else {
                    res += name + ':';
                    continue;
                }
                res = res.substr(0, res.size() - name.size()); //erase name
                while (i < n && (isalnum((unsigned char)coopCode[i]) || coopCode[i] == '_')) {
                    method += coopCode[i];
                    i++;
                }
                while (i < n && coopCode[i] != ')') {
                    method += coopCode[i];
                    i++;
                }
                
                std::string expr;
                size_t pos = method.find('(');
                bool cond = false;
                size_t k = 1;
                for (; pos+k<n && isspace((unsigned char)method[pos+k]); k++) {}
                cond = method[pos+k] == ')';
                if (cond) {
                    res += method.substr(0, pos) + "(&" + name + ")";
                    continue;
                }
                res += method.substr(0, pos) + "(&" + name + "," + method.substr(pos+1);
                for (i; i < n && isspace((unsigned char)coopCode[i]); i++) {}
                continue;
            }
            
            if (isWord(coopCode.c_str(), i, "new")) {
                i+=3;
                size_t j = i;
                std::string type;
                if (coopCode[j++] == '(') for(; j < n && coopCode[j] != ')';j++) {
                    type += coopCode[j];
                } else {
                    res += "new";
                    continue;
                }
                res += "malloc(sizeof(" + type  + "))";
                i = j+1;
                for (i; i < n && isspace((unsigned char)coopCode[i]); i++) {}
                continue;
            }
            if (isWord(coopCode.c_str(), i, "delete")) {
                i+=6;
                size_t j = i;
                std::string type;
                if (coopCode[j++] == '(') for(; j < n && coopCode[j] != ')';j++) {
                    type += coopCode[j];
                } else {
                    res += "delete";
                    continue;
                }
                res += "free(" + type  + ")";
                i = j+1;
                for (i; i < n && isspace((unsigned char)coopCode[i]); i++) {}
                continue;
            } if (isWord(coopCode.c_str(), i, "generic_class")) {
                log("found generic class\n");
                std::string def;
                i += 14;
                std::string name;
                for (;i < n && coopCode[i] != '<'; i++) {
                    name += coopCode[i];
                }
                std::string arg_name;
                for (i++;i < n && coopCode[i] != '>'; i++) {
                    arg_name += coopCode[i];
                }
                i++;
                name = replace(name, "::", "_");
                i++;
                name = trim(name);
                std::string structdef;
                size_t j = 1;
                for (i++;i < n && j != 0; i++) {
                    if (coopCode[i] == '}') j--;
                    else if (coopCode[i] == '{') j++;
                    if (j != 0) structdef += coopCode[i];
                }
                i++;
                def += replace(structdef+"}", "::", "_");
                (*generics)[name] = {def, arg_name};
                log(name << ": " << (*generics)[name] << '\n');
                for (i; i < n && isspace((unsigned char)coopCode[i]); i++) {}
                continue;
            }
            if (c == '$') {
                size_t j = i;
                i++;
                std::string var_name;
                bool cond = false;
                log("cond=" << (cond=(i < n && (isalpha((unsigned char)coopCode[i]) || coopCode[i] == '_'))) << " is ident first char\n");
                if (cond) {
                    var_name += coopCode[i++];
                } else {
                    res += '$';
                    continue;
                }
                log("cond=" << (cond=(i < n && (isalnum((unsigned char)coopCode[i]) || coopCode[i] == '_'))) << " is ident char\n");
                for (;i < n && (isalnum((unsigned char)coopCode[i]) || coopCode[i] == '_'); i++) {
                    var_name += coopCode[i];
                }

                log("var_name: '" << var_name << "'\n");

                if (var_name == "i") {
                    res += "$i";
                    continue;
                }
                while (i < n && isspace((unsigned char)coopCode[i])) {
                    i++;
                }
                log("cond=" << (cond= coopCode[i] == '=') << " is = char\n");
                log("cond=" << (variables->contains(var_name)) << " contains var\n");
                if (cond) {
                    (*variables)[var_name] = "";
                    std::string val;
                    for (i++; coopCode[i]!='\n' && i < n;i++) {
                        val += coopCode[i];
                    }
                    log("val=" << trim(val));
                    (*variables)[var_name] = trim(val);
                } else if (variables->contains(var_name)) {
                    res += (*variables)[var_name];
                } else {
                    res += "$" + var_name;
                }
                log("\n\n");
                for (i; i < n && isspace((unsigned char)coopCode[i]); i++) {}
                continue;
            }
        }


        if (!inBlockComment && !inComment)res += c;
        i++;
    }
    log("first pass\n");
    return res;
}

std::string translate_second(const std::string &coopCode, generic_classes_t *generics, transpile_time_variables_t *variables, tasks_t *tasks) {
    std::string res{}, actualNamespace{};
    size_t i = 0, n = coopCode.size();
    bool inStr = false, inChar = false, inComment = false, inBlockComment = false;
    while (i < n) {
        char c = coopCode[i];
        if (c == '"' && ! scaped(coopCode.c_str(), i) && ! inChar && !inComment && !inBlockComment) {
            inStr = !inStr;
        }

        if (c == '\'' && ! scaped(coopCode.c_str(), i) && ! inStr && !inComment && !inBlockComment) {
            inChar = !inChar;
        }

        if (c == '/' && i+1 < n && coopCode[i+1] == '/' && !inChar && ! inStr && !inComment && !inBlockComment) {
            inComment = true;
        }

        if (c == '/' && i+1 < n && coopCode[i+1] == '*' && !inChar && ! inStr && !inComment && !inBlockComment) {
            inBlockComment = true;
        }

        if (c == '\n' && inComment) {
            inComment = false;
        }

        if (c == '*' && inBlockComment && i+1 < n && coopCode[i+1] == '/') {
            inBlockComment = false;
            log("found block comment end\n");
            i+= 2;
            continue;
        }
        if (!(inStr || inChar || inComment || inBlockComment)) {
            if (isWord(coopCode.c_str(), i, "allowapply")) {
                log("found apply\n");
                i+=10;
                std::string type;
                std::string applyname;
                std::string name;
                for (;i < n && coopCode[i] != '<'; i++) {
                    name += coopCode[i];
                }
                name = replace(trim(name), "::", "_");
                for (i++;i < n && coopCode[i] != '>'; i++) {
                    type += coopCode[i];
                }
                type = replace(trim(type), "::", "_");
                for (i++;i < n && coopCode.substr(i, 2) != "as"; i++) {}
                i+=2;
                for (i++;i < n && coopCode[i] != '\n'; i++) {
                    applyname += coopCode[i];
                }
                applyname = replace(trim(applyname), "::", "_");
                if (generics->find(name) != generics->end()) {
                    auto [def, arg_name] = (*generics)[name];
                    std::string inst = replace(replace(def, "::", "_"), arg_name.c_str(), type.c_str());
                    res += "typedef struct " + applyname + " " + applyname + "; " + "struct " + applyname + "{" + inst + ";";
                } else {
                    res += "/* could not find generic class " + name + " to apply with type " + type + " and apply name " + applyname + " */";
                }
                for (i; i < n && isspace((unsigned char)coopCode[i]); i++) {}
                continue;
            }
        }

        if (!inBlockComment && !inComment)res += c;
        i++;
    }
    log("second pass\n");
    return res;
}

std::string translate_third(const std::string &coopCode, generic_classes_t *generics, transpile_time_variables_t *variables, tasks_t *tasks) {
    std::string res{}, actualNamespace{};
    size_t i = 0, n = coopCode.size();
    bool inStr = false, inChar = false, inComment = false, inBlockComment = false;
    pragma_t pragma{};
    while (i < n) {
        char c = coopCode[i];
        if (c == '"' && ! scaped(coopCode.c_str(), i) && ! inChar && !inComment && !inBlockComment) {
            inStr = !inStr;
        }

        if (c == '\'' && ! scaped(coopCode.c_str(), i) && ! inStr && !inComment && !inBlockComment) {
            inChar = !inChar;
        }

        if (c == '/' && i+1 < n && coopCode[i+1] == '/' && !inChar && ! inStr && !inComment && !inBlockComment) {
            inComment = true;
        }

        if (c == '/' && i+1 < n && coopCode[i+1] == '*' && !inChar && ! inStr && !inComment && !inBlockComment) {
            inBlockComment = true;
        }

        if (c == '\n' && inComment) {
            inComment = false;
        }

        if (c == '*' && inBlockComment && i+1 < n && coopCode[i+1] == '/') {
            inBlockComment = false;
            i+= 2; continue;
        }

        if (!(inStr || inChar || inComment || inBlockComment)) {
            if (isWord(coopCode.c_str(), i, "repeat")) {
                i+=6;
                std::string timesstr;
                if (coopCode[i++] != '(') {
                    res += "repeat";
                    continue;
                }
                for (;i < n && coopCode[i] != ','; i++) {
                    timesstr += coopCode[i];
                }
                timesstr = trim(timesstr);
                log("timesstr: '" << timesstr << "'" << '\n');
                size_t times = std::stoul(timesstr);
                std::string expr;
                size_t parents = 1;
                for (i+=2;i < n && parents != 0; i++) {
                    if (coopCode[i] == '(') parents++;
                    else if (coopCode[i] == ')') parents--;
                    if (parents != 0) expr += coopCode[i];
                }
                if (coopCode[i] == ')') log("found closing parenthesis\n");
                if (times == 0) continue;
                std::string expanded;
                for (size_t j=0; j < times; j++) {
                    expanded += replace(expr, "$i", std::to_string(j).c_str());
                }
                log("expanded: '" << expanded << "'" << '\n');
                res += trim(expanded);
                for (i; i < n && isspace((unsigned char)coopCode[i]); i++) {}
                continue;
            }
            if (coopCode.substr(i,12) == "@pragma once") {
                i+=13;
                std::string name;
                for(i; coopCode[i] != '\n' && i < n;i++) {
                    name += coopCode[i];
                }
                name = trim(name);
                pragma.name = name;
                pragma.active = true;
                for (i; i < n && isspace((unsigned char)coopCode[i]); i++) {}
                continue;
            }
            if (coopCode.substr(i,8) == "@define ") {
                i+=8;
                std::string name;
                for(i; coopCode[i] != '=' && i < n;i++) {
                    name += coopCode[i];
                }
                name = trim(name);
                std::string val;
                for(i++; coopCode.substr(i, 11)!="@end_define" && i < n;i++) {
                    val += coopCode[i];
                }
                for (i; i < n && isspace((unsigned char)coopCode[i]); i++) {}
                
                res += "#define " + name + " " + replace(trim(val), "\n", "\\\n");
                i+=11;
                continue;
            }
            if (coopCode.substr(i,4) == "@msg") {
                i+=4;
                std::string msg;
                for(i; coopCode[i] != '\n' && i < n;i++) {
                    msg += coopCode[i];
                }
                msg = trim(msg);
                std::cout << "msg: '" << msg << "'" << '\n';
                for (i; i < n && isspace((unsigned char)coopCode[i]); i++) {}
                continue;
            }
            if (isWord(coopCode.c_str(), i, "task")) {
                log("found task\n");
                i+=4;
                for (;i < n && isspace((unsigned char)coopCode[i]); i++) {}
                std::string name;
                std::string line;
                if (i < n && (isalpha((unsigned char)coopCode[i]) || coopCode[i] == '_')) {
                    name += coopCode[i];
                }
                for (;i < n && (isalnum((unsigned char)coopCode[i]) || coopCode[i] == '_'); i++) {
                    name += coopCode[i];
                }
                for (i; i < n && isspace((unsigned char)coopCode[i]); i++) {}
                enum {
                    EQUALS,
                    CONCAT,
                    RUN
                } mode = EQUALS;
                if (coopCode[i] == '=') {
                    mode = EQUALS;
                    i++;
                } if (coopCode.substr(i, 2) == "++") {
                    mode = CONCAT;
                    i+=2;
                } else if (coopCode.substr(i, 3) == "run") {
                    mode = RUN;
                    i+=3;
                }
                for (i; mode !=RUN && i < n && coopCode[i] != '\n'; i++) {
                    line += coopCode[i];
                }
                line = trim(line);
                name = trim(name);
                if (mode == EQUALS) {
                    (*tasks)[name] = line;
                } else if (mode == CONCAT) {
                    if (tasks->contains(name)) {
                        (*tasks).at(name) = line +(*tasks).at(name) ;
                    } else {
                        std::cerr << "error: cannot concatenate to task '" << name << "' because it does not exist\n";
                        std::abort();
                    }
                } else if (mode == RUN) {
                    if (tasks->contains(name)) {
                        std::string lines = (*tasks).at(name);
                        log("running task '" << name << "' with lines:\n" << lines << '\n');
                        res += lines;
                    } else {
                        std::cerr << "error: cannot run task '" << name << "' because it does not exist\n";
                        std::abort();
                    }
                }
                continue;
            }
        }
        
        if (!inBlockComment && !inComment)res += c;
        i++;
    }

    if (pragma.active) {
        res = "#ifndef " + pragma.name + "\n#define " + pragma.name + " 1\n" + res + "\n#endif";
    }
    log("third pass\n");
    return res;
}

template <typename T, typename... U>
const T& none(const T& a, const U&&...) {
    return a;
}
#define translates(coopCode, info) translate_third(translate_second(translate_first(coopCode, info->generics.get(), info->variables.get(), info->tasks.get()), info->generics.get(), info->variables.get(), info->tasks.get()), info->generics.get(), info->variables.get(), info->tasks.get())

std::string translate(const std::string &coopCode, info_t *info) {
    return translates(coopCode, info);
};

#undef translates