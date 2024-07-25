#pragma once

#include <string>
#include <sstream>
#include <iomanip>

/*
 * Преобразовать 4 байта в формате big endian в int
 */
int BytesToInt(std::string_view bytes);

std::string IntToBytes(int number);


/*
 * Расчет SHA1 хеш-суммы. Здесь в результате подразумевается не человеко-читаемая строка, а массив из 20 байтов
 * в том виде, в котором его генерирует библиотека OpenSSL
 */
std::string CalculateSHA1(const std::string& msg);

/*
 * Представить массив байтов в виде строки, содержащей только символы, соответствующие цифрам в шестнадцатеричном исчислении.
 */
std::string HexEncode(const std::string& input){
    std::ostringstream result;
    result << std::hex <<std::setfill('0');

    for (size_t i = 0; i < input.size(); ++i) {
        result << std::setw(2)<<static_cast<unsigned int>(input[i]);
    }
    
    return result.str();
}

