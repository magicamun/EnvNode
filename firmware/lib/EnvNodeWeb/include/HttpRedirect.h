// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
// See the repository root LICENSE.
#pragma once

namespace EnvNode {

// HTTP protocol fields must never pass through the UI translation catalog.
// Accept the server interface so the same response can be verified on the host.
template <typename HttpServer>
void sendSeeOther(HttpServer& server, const char* target) {
    server.sendHeader("Location", target, true);
    server.send(303, "text/plain", "See Other");
}

} // namespace EnvNode
