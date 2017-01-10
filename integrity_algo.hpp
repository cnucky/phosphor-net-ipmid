#pragma once

#include <openssl/sha.h>
#include <array>
#include <vector>

namespace cipher
{

namespace integrity
{

using Buffer = std::vector<uint8_t>;
using Key = std::array<uint8_t, SHA_DIGEST_LENGTH>;

/*
 * RSP needs more keying material than can be provided by session integrity key
 * alone. As a result all keying material for the RSP integrity algorithms
 * will be generated by processing a pre-defined set of constants using HMAC per
 * [RFC2104], keyed by SIK. These constants are constructed using a hexadecimal
 * octet value repeated up to the HMAC block size in length starting with the
 * constant 01h. This mechanism can be used to derive up to 255
 * HMAC-block-length pieces of keying material from a single SIK. For the
 * mandatory integrity algorithm HMAC-SHA1-96, processing the following
 * constant will generate the required amount of keying material.
 */
constexpr Key const1 = { 0x01, 0x01, 0x01, 0x01, 0x01,
                         0x01, 0x01, 0x01, 0x01, 0x01,
                         0x01, 0x01, 0x01, 0x01, 0x01,
                         0x01, 0x01, 0x01, 0x01, 0x01
                       };

/*
 * @enum Integrity Algorithms
 *
 * The Integrity Algorithm Number specifies the algorithm used to generate the
 * contents for the AuthCode “signature” field that accompanies authenticated
 * IPMI v2.0/RMCP+ messages once the session has been established. If the
 * Integrity Algorithm is none the AuthCode value is not calculated and the
 * AuthCode field in the message is not present.
 */
enum class Algorithms : uint8_t
{
    NONE,                  // Mandatory
    HMAC_SHA1_96,          // Mandatory
    HMAC_MD5_128,          // Optional
    MD5_128,               // Optional
    HMAC_SHA256_128,       // Optional
};

/*
 * @class Interface
 *
 * Interface is the base class for the Integrity Algorithms.
 * Unless otherwise specified, the integrity algorithm is applied to the packet
 * data starting with the AuthType/Format field up to and including the field
 * that immediately precedes the AuthCode field itself.
 */
class Interface
{
    public:
        /*
         * @brief Constructor for Interface
         *
         * @param[in] - Session Integrity Key to generate K1
         * @param[in] - Additional keying material to generate K1
         * @param[in] - AuthCode length
         */
        explicit Interface(const Buffer& sik,
                           const Key& addKey,
                           size_t authLength);

        Interface() = delete;
        virtual ~Interface() = default;
        Interface(const Interface&) = default;
        Interface& operator=(const Interface&) = default;
        Interface(Interface&&) = default;
        Interface& operator=(Interface&&) = default;

        /*
         * @brief Verify the integrity data of the packet
         *
         * @param[in] packet - Incoming IPMI packet
         * @param[in] packetLen - Packet length excluding authCode
         * @param[in] integrityData - Iterator to the authCode in the packet
         *
         * @return true if authcode in the packet is equal to one generated
         *         using integrity algorithm on the packet data, false otherwise
         */
        bool virtual verifyIntegrityData(
                const Buffer& packet,
                const size_t packetLen,
                Buffer::const_iterator integrityData) const = 0;

        /*
         * @brief Generate integrity data for the outgoing IPMI packet
         *
         * @param[in] input - Outgoing IPMI packet
         *
         * @return authcode for the outgoing IPMI packet
         *
         */
        Buffer virtual generateIntegrityData(const Buffer& input) const = 0;

        /*
         * AuthCode field length varies based on the integrity algorithm, for
         * HMAC-SHA1-96 the authcode field is 12 bytes. For HMAC-SHA256-128 and
         * HMAC-MD5-128 the authcode field is 16 bytes.
         */
        size_t authCodeLength;

    protected:

        // K1 key used to generated the integrity data
        Key K1;
};

/*
 * @class AlgoSHA1
 *
 * @brief Implementation of the HMAC-SHA1-96 Integrity algorithm
 *
 * HMAC-SHA1-96 take the Session Integrity Key and use it to generate K1. K1 is
 * then used as the key for use in HMAC to produce the AuthCode field.
 * For “one-key” logins, the user’s key (password) is used in the creation of
 * the Session Integrity Key. When the HMAC-SHA1-96 Integrity Algorithm is used
 * the resulting AuthCode field is 12 bytes (96 bits).
 */
class AlgoSHA1 final : public Interface
{
    public:
        static constexpr size_t SHA1_96_AUTHCODE_LENGTH = 12;

        /*
         * @brief Constructor for AlgoSHA1
         *
         * @param[in] - Session Integrity Key
         */
        explicit AlgoSHA1(const Buffer& sik) :
            Interface(sik, const1, SHA1_96_AUTHCODE_LENGTH) {}

        AlgoSHA1() = delete;
        ~AlgoSHA1() = default;
        AlgoSHA1(const AlgoSHA1&) = default;
        AlgoSHA1& operator=(const AlgoSHA1&) = default;
        AlgoSHA1(AlgoSHA1&&) = default;
        AlgoSHA1& operator=(AlgoSHA1&&) = default;

        /*
         * @brief Verify the integrity data of the packet
         *
         * @param[in] packet - Incoming IPMI packet
         * @param[in] length - Length of the data in the packet to calculate
         *                     the integrity data
         * @param[in] integrityData - Iterator to the authCode in the packet
         *
         * @return true if authcode in the packet is equal to one generated
         *         using integrity algorithm on the packet data, false otherwise
         */
        bool verifyIntegrityData(
                const Buffer& packet,
                const size_t length,
                Buffer::const_iterator integrityData) const override;

        /*
         * @brief Generate integrity data for the outgoing IPMI packet
         *
         * @param[in] input - Outgoing IPMI packet
         *
         * @return on success return the integrity data for the outgoing IPMI
         *         packet
         */
        Buffer generateIntegrityData(const Buffer& packet) const override;

    private:
        /*
         * @brief Generate HMAC based on HMAC-SHA1-96 algorithm
         *
         * @param[in] input - pointer to the message
         * @param[in] length - length of the message
         *
         * @return on success returns the message authentication code
         *
         */
        Buffer generateHMAC(const uint8_t* input, const size_t len) const;
};

}// namespace integrity

}// namespace cipher

