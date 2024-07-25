#include "bencode.h"

namespace Bencode {
    void check_index(std::string& file, size_t& cur_index){
        if(cur_index >= file.size()){
            throw std::range_error("index out of range");
        }
    }

    int64_t parsing_integer(std::string& file, size_t& cur_index, const char end){
        int64_t result = 0;
        bool is_negative = false;
        
        check_index(file, cur_index);

        if(file[cur_index] == 'i') ++cur_index;
        for(;file[cur_index] != end; ++cur_index){
            if(file[cur_index] == '-') is_negative = true;
            result *= 10;
            result += file[cur_index] - '0';
        }
        
        if(file[cur_index] == end) ++cur_index;
        return result * (is_negative?-1:1);
    }
    std::string parsing_string(std::string& file, size_t& cur_index){
        std::string result = "";

        check_index(file, cur_index);
        size_t size = parsing_integer(file, cur_index, ':');

        for (size_t i = 0; i < size; i++)
        {
            result += file[cur_index + i];
        }
        cur_index += size;
        return result;
    }

    std::vector<std::any> parsing_list(std::string& file, size_t& cur_index){
        std::vector<std::any> result;
        check_index(file, cur_index);
        if(file[cur_index] == 'l') ++cur_index;

        for(;file[cur_index] != 'e';){
            result.push_back(parsing(file, cur_index));
        }
        if(file[cur_index] == 'e') ++cur_index;

        return result;
    }

    std::map<std::string, std::any> parsing_dictionary(std::string& file, size_t& cur_index){
        std::map<std::string, std::any> result;
        check_index(file, cur_index);

        if(file[cur_index] == 'd') ++cur_index;

        for(;file[cur_index] != 'e';){
            std::string key = parsing_string(file, cur_index);
            std::any value = parsing(file, cur_index);
            result.insert({key, value});
            check_index(file, cur_index);
        }

        if(file[cur_index] == 'e') ++cur_index;
        return result;
    }

    std::any parsing(std::string& file, size_t& cur_index){
        check_index(file, cur_index);
        switch (file[cur_index])
        {
        case 'i':
            return parsing_integer(file, cur_index, 'e');
            break;
        case 'd':
            return parsing_dictionary(file, cur_index);
            break;
        case 'l':
            return parsing_list(file, cur_index);
            break;
        default:
            return parsing_string(file, cur_index);
            break;
        }
    }

    std::string ecoding(std::any object){
        std::string code;
        if(object.type() == typeid(int64_t)){
            code +='i';
            code += std::to_string(std::any_cast<int64_t>(object));
            code += 'e';
        }
        if(object.type() == typeid(std::string)){
            std::string value = std::any_cast<std::string>(object);
            code += std::to_string(value.size());
            code += ':';
            code += value;
        }
        if(object.type() == typeid(std::vector<std::any>)){
            code += 'l';
            std::vector<std::any> value = std::any_cast<std::vector<std::any>>(object);
            for (auto &&i : value)
            {
                code+= ecoding(i);
            }
            code += 'e';

        }
        if(object.type() == typeid(std::map<std::string, std::any>)){
            code += 'd';
            std::map<std::string, std::any> value = std::any_cast<std::map<std::string, std::any>>(object);
            for (auto &&[key, value] : value)
            {
                code += ecoding(key);
                code += ecoding(value);
            }
            code += 'e';
        }
        return code;
    }    
}
