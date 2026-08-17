//---------------------------------------------------------------------------

#ifndef ComputerIdentityH
#define ComputerIdentityH

#include <optional>
#include <string>

// Win32_ComputerSystemProduct.UUID via WMI, already canonicalized.
// Returns nullopt when WMI fails or the UUID is unusable (empty/all-zero/all-F).
std::optional<std::string> QueryComputerSystemProductUuid();

#endif
