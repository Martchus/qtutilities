#ifndef QT_UTILITIES_SETUP_VERIFICATION_H
#define QT_UTILITIES_SETUP_VERIFICATION_H

#include <QByteArray>

#include <openssl/ec.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <string>
#include <string_view>
#include <vector>

namespace QtUtilities {

namespace Detail {
/// \brief Initializes OpenSSL.
/// \remarks This function is an implementation detail and must not be called by users of the qtutilities library.
inline void initOpenSsl()
{
    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();
}

/// \brief Returns the current OpenSSL error.
/// \remarks This function is an implementation detail and must not be called by users of the qtutilities library.
inline std::string getOpenSslError()
{
    const auto errCode = ERR_get_error();
    if (errCode == 0) {
        return "unknown OpenSSL error";
    }
    auto buffer = std::array<char, 256>();
    ERR_error_string_n(errCode, buffer.data(), buffer.size());
    return std::string(buffer.data());
}

/// \brief Extracts the base64-encoded body from a PEM block.
/// \remarks This function is an implementation detail and must not be called by users of the qtutilities library.
inline QByteArray extractPemBody(std::string_view pem, std::string_view header)
{
    auto body = QByteArray();
    auto begin = pem.find(header);
    if (begin == std::string_view::npos) {
        return body;
    }
    begin += header.size();

    auto end = pem.find("-----END", begin);
    if (end == std::string_view::npos) {
        return body;
    }

    body = QByteArray(pem.data() + begin, static_cast<qsizetype>(end - begin));
    body.erase(std::remove_if(body.begin(), body.end(), ::isspace), body.end());
    return body;
}

/// \brief Converts PEM-encoded ECDSA signature into DER-encoded signature.
/// \remarks This function is an implementation detail and must not be called by users of the qtutilities library.
inline std::string parsePemEcdsaSignature(std::string_view pemSignature, std::vector<unsigned char> &decodedSignature)
{
    const auto pemSignatureBody = extractPemBody(pemSignature, "-----BEGIN SIGNATURE-----");
    if (pemSignatureBody.isEmpty()) {
        return "invalid or missing PEM signature block";
    }
    auto derSignature = QByteArray::fromBase64Encoding(pemSignatureBody);
    if (!derSignature) {
        return "unable to decode PEM signature block";
    }
    const auto *p = reinterpret_cast<const unsigned char *>((*derSignature).data());
    const auto size = static_cast<std::size_t>((*derSignature).size());
    decodedSignature.insert(decodedSignature.end(), p, p + size);
    return std::string();
}

} // namespace Detail

/*!
 * \brief Verifies \a data with the specified \a publicKeyPem and \a signaturePem.
 * \returns Returns an empty string if \a data is valid and an error message otherwise.
 * \remarks This function is experimental and might be changed in incompatible ways (API and ABI wise) or completely removed
 *          in further minor/patch releases.
 *
 * A key pair for signing can be created with the following commands:
 * ```
 * openssl ecparam -name secp521r1 -genkey -noout -out release-signing-private-openssl-secp521r1.pem
 * openssl ec -in release-signing-private-openssl-secp521r1.pem -pubout > release-signing-public-openssl-secp521r1.pem
 * ```
 *
 * A signature can be created an verified using the following commands:
 * ```
 * openssl dgst -sha256 -sign release-signing-private-openssl-secp521r1.pem test_msg.txt > test_msg-secp521r1.txt.sig
 * openssl dgst -sha256 -verify release-signing-public-openssl-secp521r1.pem -signature test_msg-secp521r1.txt.sig test_msg.txt
 * ```
 *
 * The signature can be converted to pem format using the following command:
 * ```
 * cat test_msg-secp521r1.txt.sig | base64 -w 64
 * ```
 */
inline std::string verifySignature(std::string_view publicKeyPem, std::string_view signaturePem, std::string_view data)
{
    auto error = std::string();
    Detail::initOpenSsl();

    auto derSignature = std::vector<unsigned char>();
    if (error = Detail::parsePemEcdsaSignature(signaturePem, derSignature); !error.empty()) {
        return error;
    }

    BIO *const keyBio = BIO_new_mem_buf(publicKeyPem.data(), static_cast<int>(publicKeyPem.size()));
    if (!keyBio) {
        return error = "BIO_new_mem_buf failed: " + Detail::getOpenSslError();
    }

    EVP_PKEY *const publicKey = PEM_read_bio_PUBKEY(keyBio, nullptr, nullptr, nullptr);
    BIO_free(keyBio);
    if (!publicKey) {
        return error = "PEM_read_bio_PUBKEY failed: " + Detail::getOpenSslError();
    }

    EVP_MD_CTX *const mdCtx = EVP_MD_CTX_new();
    if (!mdCtx) {
        EVP_PKEY_free(publicKey);
        return error = "EVP_MD_CTX_new failed: " + Detail::getOpenSslError();
    }

    if (EVP_DigestVerifyInit(mdCtx, nullptr, EVP_sha256(), nullptr, publicKey) != 1) {
        error = "EVP_DigestVerifyInit failed: " + Detail::getOpenSslError();
    } else if (EVP_DigestVerifyUpdate(mdCtx, data.data(), data.size()) != 1) {
        error = "EVP_DigestVerifyUpdate failed: " + Detail::getOpenSslError();
    } else {
        switch (EVP_DigestVerifyFinal(mdCtx, derSignature.data(), derSignature.size())) {
        case 0:
            error = "incorrect signature";
            break;
        case 1:
            break; // signature is correct
        default:
            error = "EVP_DigestVerifyFinal failed: " + Detail::getOpenSslError();
            break;
        }
    }

    EVP_MD_CTX_free(mdCtx);
    EVP_PKEY_free(publicKey);
    return error;
}

} // namespace QtUtilities

#endif // QT_UTILITIES_SETUP_VERIFICATION_H
