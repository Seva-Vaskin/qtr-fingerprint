#pragma once

#include <string>
#include <iostream>

#include <glog/logging.h>

/**
 * Check, if string a ends with string b.
 * @param a
 * @param b
 * @return true, if b is end of string a, false otherwise.
 */
bool endsWith(const std::string &a, const std::string &b);


/**
 * Print message if argument is empty and finish program in that case
 * @param argument
 * @param message
 */
void emptyArgument(const std::string &argument, const std::string &message);
