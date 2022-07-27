// SPDX-FileCopyrightText: 2017 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <string>

namespace WebService {

struct WebResult;

class Client {
public:
    Client(std::string host, std::string username, std::string token);
    ~Client();

    /**
     * Posts JSON to the specified path.
     * @param path the URL segment after the host address.
     * @param data String of JSON data to use for the body of the POST request.
     * @param allow_anonymous If true, allow anonymous unauthenticated requests.
     * @return the result of the request.
     */
    WebResult PostJson(const std::string& path, const std::string& data, bool allow_anonymous);

    /**
     * Gets JSON from the specified path.
     * @param path the URL segment after the host address.
     * @param allow_anonymous If true, allow anonymous unauthenticated requests.
     * @return the result of the request.
     */
    WebResult GetJson(const std::string& path, bool allow_anonymous);

    /**
     * Deletes JSON to the specified path.
     * @param path the URL segment after the host address.
     * @param data String of JSON data to use for the body of the DELETE request.
     * @param allow_anonymous If true, allow anonymous unauthenticated requests.
     * @return the result of the request.
     */
    WebResult DeleteJson(const std::string& path, const std::string& data, bool allow_anonymous);

    /**
     * Gets a plain string from the specified path.
     * @param path the URL segment after the host address.
     * @param allow_anonymous If true, allow anonymous unauthenticated requests.
     * @return the result of the request.
     */
    WebResult GetPlain(const std::string& path, bool allow_anonymous);

    /**
     * Gets an PNG image from the specified path.
     * @param path the URL segment after the host address.
     * @param allow_anonymous If true, allow anonymous unauthenticated requests.
     * @return the result of the request.
     */
    WebResult GetImage(const std::string& path, bool allow_anonymous);

    /**
     * Requests an external JWT for the specific audience provided.
     * @param audience the audience of the JWT requested.
     * @return the result of the request.
     */
    WebResult GetExternalJWT(const std::string& audience);

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace WebService
