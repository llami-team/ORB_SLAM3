#ifndef BASE64_H
#define BASE64_H

#include <string>
#include <vector>

/**
 * @brief Encodes binary data to base64 string
 * 
 * @param in Vector of bytes to encode
 * @return std::string Base64 encoded string
 */
std::string base64_encode(const std::vector<uint8_t> &in);

/**
 * @brief Decodes base64 string to binary data
 * 
 * @param in Base64 encoded string
 * @return std::vector<uint8_t> Decoded binary data
 */
std::vector<uint8_t> base64_decode(const std::string &in);

#endif // BASE64_H
