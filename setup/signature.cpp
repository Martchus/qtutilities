#include "./signature.h"

#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
#include <c++utilities/io/ansiescapecodes.h>
#include <c++utilities/io/misc.h>

#include <QString>

#include <iostream>
#endif

#include "resources/config.h"

#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
// clang-format off
constexpr auto defaultSigningKey = std::string_view(
R"(-----BEGIN PUBLIC KEY-----
MIGbMBAGByqGSM49AgEGBSuBBAAjA4GGAAQBzGxkQSS43eE4r+A7HjlcEch5apsn
fKOgJWaRE2TOD9dNoBO2RSaJEAzzOXg2BPMsiPdr+Ty99FZtX8fmIcgJHGoB3sE1
PmSOaw3YWAXrHUYslrVRJI4iYCLuT4qjFMHgmqvphEE/zGDZ5Tyu6FwVlSjCO4Yy
FdsjpzKV6nrX6EsK++o=
-----END PUBLIC KEY-----
)");
// clang-format on
#else
constexpr auto defaultSigningKey = std::string_view();
#endif

namespace QtUtilities {

std::string_view publicSigningKey()
{
#ifdef QT_UTILITIES_SETUP_TOOLS_ENABLED
    static const auto customSigningKey = [] {
        const auto path = qEnvironmentVariable(PROJECT_VARNAME_UPPER "_PUBLIC_SIGNING_KEY").toStdString();
        if (path.empty()) {
            return std::string();
        }
        try {
            return CppUtilities::readFile(path);
        } catch (const std::ios_base::failure &e) {
            std::cerr << CppUtilities::EscapeCodes::Phrases::Error << "Unable to read custom signing key from \"" << path << "\": " << e.what()
                      << CppUtilities::EscapeCodes::Phrases::End;
            return std::string();
        }
    }();
    if (!customSigningKey.empty()) {
        return customSigningKey;
    }
#endif
    return defaultSigningKey;
}

} // namespace QtUtilities
