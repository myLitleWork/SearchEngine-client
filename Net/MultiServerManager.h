#pragma once

#include "myAsioClient.h"

#include <nlohmann/json.hpp>

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

struct ServerEndpoint
{
	std::string id;
	std::string displayName;
	std::string host;
	std::string port;
	bool enabled{true};
	std::string year; // Client-configured service year; never sent in search requests.
};

// Local flash uses AUTHENTICATE_V1; admin984.bin stays on USER_REGISTRY.
enum class ServerAuthPolicy
{
	LegacyUserRegistry = 0,
	AuthenticateV1 = 1
};

struct ServerAuthAttempt
{
	std::string serverId;
	std::string displayName;
	bool ok{false};
	std::string error;
	bool hasWireErrorCode{false};
	std::uint32_t wireErrorCode{0};
};

inline std::string unauthorizedSearchSkipMessage()
{
	return "Нет авторизованной сессии. Сервер пропущен; остальные сессии остаются.";
}

class ServerSession
{
public:
	explicit ServerSession(ServerEndpoint endpoint)
		: endpoint_(std::move(endpoint))
	{
	}

	const ServerEndpoint& endpoint() const noexcept { return endpoint_; }

	void setAuthPolicy(ServerAuthPolicy policy)
	{
		std::lock_guard<std::mutex> lock(requestMutex_);
		authPolicy_ = policy;
	}

	ServerAuthPolicy authPolicy() const
	{
		std::lock_guard<std::mutex> lock(requestMutex_);
		return authPolicy_;
	}

	bool isServerAuthorized() const
	{
		std::lock_guard<std::mutex> lock(requestMutex_);
		return serverAuthorized_;
	}

	bool connect(const std::string& user, std::string* errMsg = nullptr)
	{
		std::lock_guard<std::mutex> lock(requestMutex_);
		return ensureConnectedLocked(user, errMsg);
	}

	bool reconnect(const std::string& user, std::string* errMsg = nullptr)
	{
		std::lock_guard<std::mutex> lock(requestMutex_);
		client_.disConnect();
		registeredUser_.clear();
		serverAuthorized_ = false;
		return ensureConnectedLocked(user, errMsg);
	}

	void disconnect()
	{
		std::lock_guard<std::mutex> lock(requestMutex_);
		client_.disConnect();
		registeredUser_.clear();
		serverAuthorized_ = false;
	}

	bool isConnected()
	{
		std::lock_guard<std::mutex> lock(requestMutex_);
		return client_.isConnected();
	}

	// Device path only. Never call for admin984.bin / LegacyUserRegistry.
	bool authenticateV1(const std::string& clientName,
						const std::string& clientId,
						const std::string& deviceType,
						const std::string& deviceId,
						const std::string& signature = {},
						std::string* errMsg = nullptr,
						search_protocol::ServerErrorInfo* serverError = nullptr)
	{
		std::lock_guard<std::mutex> lock(requestMutex_);
		serverAuthorized_ = false;
		if (serverError)
			serverError->reset();
		if (authPolicy_ != ServerAuthPolicy::AuthenticateV1) {
			if (errMsg)
				*errMsg = "AUTHENTICATE_V1 недоступен в режиме USER_REGISTRY";
			return false;
		}
		// Transport only: empty user must not trigger USER_REGISTRY.
		if (!ensureConnectedLocked(std::string{}, errMsg))
			return false;

		nlohmann::json body{
			{"client_id", clientId},
			{"client_name", clientName},
			{"device_type", deviceType},
			{"device_id", deviceId},
			{"signature", signature}};
		std::string reply;
		if (!client_.requestText(
				body.dump(), COMMAND::AUTHENTICATE_V1, reply, nullptr, errMsg, serverError))
		{
			// Auth failure (or server close-after-error): drop local transport.
			// Never fall back to USER_REGISTRY after AUTHENTICATE_V1 rejection.
			client_.disConnect();
			registeredUser_.clear();
			serverAuthorized_ = false;
			return false;
		}

		try {
			const auto parsed = nlohmann::json::parse(reply);
			if (!parsed.is_object() || !parsed.value("ok", false)) {
				client_.disConnect();
				registeredUser_.clear();
				serverAuthorized_ = false;
				if (errMsg)
					*errMsg = "Сервер отклонил AUTHENTICATE_V1: " + reply;
				return false;
			}
		} catch (const std::exception& ex) {
			client_.disConnect();
			registeredUser_.clear();
			serverAuthorized_ = false;
			if (errMsg) {
				*errMsg = std::string("Некорректный ответ AUTHENTICATE_V1: ") +
					ex.what();
			}
			return false;
		}

		registeredUser_ = clientName;
		serverAuthorized_ = true;
		return true;
	}

	bool requestText(const std::string& user,
					 const std::string& request,
					 COMMAND command,
					 std::string& out,
					 std::atomic_uint64_t* progress = nullptr,
					 std::string* errMsg = nullptr)
	{
		std::lock_guard<std::mutex> lock(requestMutex_);
		if (!ensureAuthorizedForRequestLocked(user, errMsg))
			return false;
		const bool ok = client_.requestText(request, command, out, progress, errMsg);
		if (!ok && !client_.isConnected()) {
			registeredUser_.clear();
			serverAuthorized_ = false;
		}
		return ok;
	}

	bool requestBinary(const std::string& user,
					   const std::vector<uint8_t>& request,
					   COMMAND command,
					   std::vector<uint8_t>& out,
					   std::atomic_uint64_t* progress = nullptr,
					   std::string* errMsg = nullptr,
					   search_protocol::ServerErrorInfo* serverError = nullptr)
	{
		std::lock_guard<std::mutex> lock(requestMutex_);
		if (serverError)
			serverError->reset();
		if (!ensureAuthorizedForRequestLocked(user, errMsg))
			return false;
		const bool ok = client_.requestBinary(
			request, command, out, progress, errMsg, serverError);
		if (!ok && !client_.isConnected()) {
			registeredUser_.clear();
			serverAuthorized_ = false;
		}
		return ok;
	}

	bool requestBinFile(const std::string& user,
						const std::string& path,
						std::vector<uint8_t>& out,
						std::atomic_uint64_t* progress = nullptr,
						std::string* errMsg = nullptr)
	{
		std::lock_guard<std::mutex> lock(requestMutex_);
		if (!ensureAuthorizedForRequestLocked(user, errMsg))
			return false;
		const bool ok = client_.requestBinFile(path, out, progress, errMsg);
		if (!ok && !client_.isConnected()) {
			registeredUser_.clear();
			serverAuthorized_ = false;
		}
		return ok;
	}

	bool requestFileToPath(const std::string& user,
						   const std::string& request,
						   COMMAND command,
						   const std::string& outPath,
						   MyAsioClient::FileProgressCallback onProgress = nullptr,
						   std::uint64_t* receivedSize = nullptr,
						   std::string* errMsg = nullptr,
						   bool flushStreamToOs = false,
						   bool replaceAfterSuccess = false)
	{
		std::lock_guard<std::mutex> lock(requestMutex_);
		if (!ensureAuthorizedForRequestLocked(user, errMsg))
			return false;
		const bool ok = client_.requestFileToPath(
			request, command, outPath, onProgress, receivedSize, errMsg, flushStreamToOs,
			replaceAfterSuccess);
		if (!ok && !client_.isConnected()) {
			registeredUser_.clear();
			serverAuthorized_ = false;
		}
		return ok;
	}

	bool uploadFileV1(const std::string& user,
					  const std::wstring& localPath,
					  COMMAND command,
					  std::string* savedName = nullptr,
					  std::string* errMsg = nullptr,
					  search_protocol::ServerErrorInfo* serverError = nullptr)
	{
		std::lock_guard<std::mutex> lock(requestMutex_);
		if (serverError)
			serverError->reset();
		if (!ensureAuthorizedForRequestLocked(user, errMsg))
			return false;
		const bool ok = client_.uploadFileV1(
			localPath, command, savedName, errMsg, serverError);
		if (!ok && !client_.isConnected()) {
			registeredUser_.clear();
			serverAuthorized_ = false;
		}
		return ok;
	}

private:
	enum class ProtocolSupport
	{
		Unknown,
		Legacy,
		TypedErrorsV1
	};

	bool requireServerAuthorizedLocked(std::string* errMsg) const
	{
		if (serverAuthorized_)
			return true;
		if (errMsg) {
			*errMsg = serverErrorUserMessage(static_cast<std::uint32_t>(
				search_protocol::ErrorCode::AuthRequired));
		}
		return false;
	}

	bool ensureAuthorizedForRequestLocked(const std::string& user, std::string* errMsg)
	{
		if (authPolicy_ == ServerAuthPolicy::AuthenticateV1) {
			// Flash: never invent server-auth via USER_REGISTRY.
			if (!ensureConnectedLocked(std::string{}, errMsg))
				return false;
			return requireServerAuthorizedLocked(errMsg);
		}
		if (!ensureConnectedLocked(user, errMsg))
			return false;
		return requireServerAuthorizedLocked(errMsg);
	}

	bool ensureConnectedLocked(const std::string& user, std::string* errMsg)
	{
		if (!client_.isConnected()) {
			std::string connectError;
			if (!client_.tryConnect(endpoint_.host, endpoint_.port, connectError)) {
				if (errMsg) *errMsg = connectError;
				serverAuthorized_ = false;
				return false;
			}
			registeredUser_.clear();
			serverAuthorized_ = false;

			if (protocolSupport_ != ProtocolSupport::Legacy) {
				std::string negotiationError;
				const auto negotiation =
					client_.negotiateProtocol(&negotiationError);

				if (negotiation ==
					MyAsioClient::ProtocolNegotiationResult::TypedErrorsV1) {
					protocolSupport_ = ProtocolSupport::TypedErrorsV1;
				}
				else if (negotiation ==
					MyAsioClient::ProtocolNegotiationResult::LegacyOnCurrentConnection) {
					protocolSupport_ = ProtocolSupport::Legacy;
				}
				else if (negotiation ==
					MyAsioClient::ProtocolNegotiationResult::LegacyReconnectRequired) {
					client_.disConnect();
					std::string legacyConnectError;
					if (!client_.tryConnect(
							endpoint_.host, endpoint_.port, legacyConnectError)) {
						if (errMsg) {
							*errMsg = "Ошибка согласования протокола (" + negotiationError +
								"); не удалось переподключиться в legacy-режиме: " + legacyConnectError;
						}
						return false;
					}
					protocolSupport_ = ProtocolSupport::Legacy;
					registeredUser_.clear();
					serverAuthorized_ = false;
				}
				else {
					client_.disConnect();
					if (errMsg) {
						*errMsg = negotiationError.empty()
							? "Не удалось согласовать протокол с сервером"
							: negotiationError;
					}
					return false;
				}
			}
		}

		if (authPolicy_ == ServerAuthPolicy::AuthenticateV1) {
			// Device sessions are authorized only by authenticateV1().
			return true;
		}

		if (!user.empty() && registeredUser_ != user) {
			std::string reply;
			std::string registryError;
			if (!client_.requestText(user, COMMAND::USER_REGISTRY, reply, nullptr, &registryError)) {
				if (errMsg) *errMsg = registryError;
				serverAuthorized_ = false;
				return false;
			}
			if (reply != "OK") {
				if (errMsg) *errMsg = "Сервер отклонил регистрацию пользователя: " + reply;
				serverAuthorized_ = false;
				return false;
			}
			registeredUser_ = user;
			serverAuthorized_ = true;
		}
		else if (!user.empty() && registeredUser_ == user) {
			serverAuthorized_ = true;
		}
		return true;
	}

	ServerEndpoint endpoint_;
	MyAsioClient client_;
	mutable std::mutex requestMutex_;
	std::string registeredUser_;
	bool serverAuthorized_{false};
	ServerAuthPolicy authPolicy_{ServerAuthPolicy::LegacyUserRegistry};
	ProtocolSupport protocolSupport_{ProtocolSupport::Unknown};
};

class MultiServerManager
{
public:
	void configure(const std::vector<ServerEndpoint>& endpoints,
				   const std::string& primaryServerId)
	{
		std::map<std::string, std::shared_ptr<ServerSession>> sessions;
		std::vector<std::string> order;
		for (const auto& endpoint : endpoints) {
			if (!endpoint.enabled || endpoint.id.empty())
				continue;
			auto session = std::make_shared<ServerSession>(endpoint);
			session->setAuthPolicy(authPolicy_);
			sessions[endpoint.id] = std::move(session);
			order.push_back(endpoint.id);
		}

		std::lock_guard<std::mutex> lock(mutex_);
		sessions_.swap(sessions);
		order_.swap(order);
		deviceAuthCache_ = {};
		primaryServerId_ = primaryServerId;
		if (sessions_.find(primaryServerId_) == sessions_.end())
			primaryServerId_ = order_.empty() ? std::string{} : order_.front();
	}

	void setAuthPolicy(ServerAuthPolicy policy)
	{
		std::vector<std::shared_ptr<ServerSession>> sessions;
		{
			std::lock_guard<std::mutex> lock(mutex_);
			authPolicy_ = policy;
			for (const auto& item : sessions_)
				sessions.push_back(item.second);
		}
		for (const auto& session : sessions)
			session->setAuthPolicy(policy);
	}

	ServerAuthPolicy authPolicy() const
	{
		std::lock_guard<std::mutex> lock(mutex_);
		return authPolicy_;
	}

	void setUser(const std::string& user)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		user_ = user;
	}

	std::string primaryServerId() const
	{
		std::lock_guard<std::mutex> lock(mutex_);
		return primaryServerId_;
	}

	std::vector<std::string> serverIds() const
	{
		std::lock_guard<std::mutex> lock(mutex_);
		return order_;
	}

	std::size_t serverCount() const
	{
		std::lock_guard<std::mutex> lock(mutex_);
		return order_.size();
	}

	std::size_t authorizedServerCount() const
	{
		std::size_t count = 0;
		for (const auto& id : serverIds()) {
			if (isServerAuthorized(id))
				++count;
		}
		return count;
	}

	bool hasAnyServerAuthorized() const
	{
		return authorizedServerCount() > 0;
	}

	bool isServerAuthorized(const std::string& serverId) const
	{
		auto session = findSession(serverId);
		return session && session->isServerAuthorized();
	}

	std::vector<std::string> authorizedServerIds() const
	{
		std::vector<std::string> result;
		for (const auto& id : serverIds()) {
			if (isServerAuthorized(id))
				result.push_back(id);
		}
		return result;
	}

	std::vector<ServerEndpoint> endpointsInOrder() const
	{
		std::lock_guard<std::mutex> lock(mutex_);
		std::vector<ServerEndpoint> result;
		result.reserve(order_.size());
		for (const auto& id : order_) {
			auto it = sessions_.find(id);
			if (it != sessions_.end())
				result.push_back(it->second->endpoint());
		}
		return result;
	}

	std::string displayName(const std::string& serverId) const
	{
		auto session = findSession(serverId);
		return session ? session->endpoint().displayName : serverId;
	}

	bool connect(const std::string& serverId, std::string* errMsg = nullptr)
	{
		auto session = findSession(serverId);
		if (!session) return missingServer(serverId, errMsg);
		return session->connect(currentUser(), errMsg);
	}

	bool connectPrimary(std::string* errMsg = nullptr)
	{
		return connect(primaryServerId(), errMsg);
	}

	bool authenticateV1(const std::string& serverId,
						const std::string& clientName,
						const std::string& clientId,
						const std::string& deviceType,
						const std::string& deviceId,
						const std::string& signature = {},
						std::string* errMsg = nullptr,
						search_protocol::ServerErrorInfo* serverError = nullptr)
	{
		auto session = findSession(serverId);
		if (!session) return missingServer(serverId, errMsg);
		const bool ok = session->authenticateV1(
			clientName, clientId, deviceType, deviceId, signature, errMsg, serverError);
		if (ok) {
			rememberDeviceAuth(clientName, clientId, deviceType, deviceId, signature);
			std::lock_guard<std::mutex> lock(mutex_);
			user_ = clientName;
		}
		return ok;
	}

	bool authenticatePrimaryV1(const std::string& clientName,
							   const std::string& clientId,
							   const std::string& deviceType,
							   const std::string& deviceId,
							   const std::string& signature = {},
							   std::string* errMsg = nullptr)
	{
		return authenticateV1(
			primaryServerId(),
			clientName,
			clientId,
			deviceType,
			deviceId,
			signature,
			errMsg);
	}

	// Device Connect: AUTHENTICATE_V1 on every enabled server for the selected group.
	std::vector<ServerAuthAttempt> authenticateAllV1(
		const std::string& clientName,
		const std::string& clientId,
		const std::string& deviceType,
		const std::string& deviceId,
		const std::string& signature = {})
	{
		std::vector<ServerAuthAttempt> attempts;
		const auto ids = serverIds();
		attempts.reserve(ids.size());
		for (const auto& id : ids) {
			ServerAuthAttempt attempt;
			attempt.serverId = id;
			attempt.displayName = displayName(id);
			search_protocol::ServerErrorInfo wireError;
			attempt.ok = authenticateV1(
				id, clientName, clientId, deviceType, deviceId, signature,
				&attempt.error, &wireError);
			if (wireError.hasCode) {
				attempt.hasWireErrorCode = true;
				attempt.wireErrorCode = wireError.rawCode;
			}
			attempts.push_back(std::move(attempt));
		}
		return attempts;
	}

	// Admin Connect: USER_REGISTRY on every enabled server for the selected group.
	// Does not use AUTHENTICATE_V1. Partial success is allowed.
	std::vector<ServerAuthAttempt> authenticateAllLegacy(const std::string& user)
	{
		if (!user.empty())
			setUser(user);

		std::vector<ServerAuthAttempt> attempts;
		const auto ids = serverIds();
		attempts.reserve(ids.size());
		if (authPolicy() == ServerAuthPolicy::AuthenticateV1) {
			for (const auto& id : ids) {
				ServerAuthAttempt attempt;
				attempt.serverId = id;
				attempt.displayName = displayName(id);
				attempt.error =
					"USER_REGISTRY недоступен в режиме AUTHENTICATE_V1";
				attempts.push_back(std::move(attempt));
			}
			return attempts;
		}

		for (const auto& id : ids) {
			ServerAuthAttempt attempt;
			attempt.serverId = id;
			attempt.displayName = displayName(id);
			attempt.ok = connect(id, &attempt.error);
			attempts.push_back(std::move(attempt));
		}
		return attempts;
	}

	bool reconnect(const std::string& serverId, std::string* errMsg = nullptr)
	{
		auto session = findSession(serverId);
		if (!session) return missingServer(serverId, errMsg);
		if (authPolicy() == ServerAuthPolicy::AuthenticateV1) {
			if (errMsg) {
				*errMsg = "Для device-сессии нужен повторный AUTHENTICATE_V1, "
					"а не USER_REGISTRY";
			}
			return false;
		}
		return session->reconnect(currentUser(), errMsg);
	}

	bool isConnected(const std::string& serverId)
	{
		auto session = findSession(serverId);
		return session && session->isConnected();
	}

	void disconnectAll()
	{
		std::vector<std::shared_ptr<ServerSession>> sessions;
		{
			std::lock_guard<std::mutex> lock(mutex_);
			deviceAuthCache_ = {};
			for (const auto& item : sessions_)
				sessions.push_back(item.second);
		}
		for (const auto& session : sessions)
			session->disconnect();
	}

	bool requestText(const std::string& serverId,
					 const std::string& request,
					 COMMAND command,
					 std::string& out,
					 std::atomic_uint64_t* progress = nullptr,
					 std::string* errMsg = nullptr)
	{
		return runPreparedRequest(serverId, errMsg,
			[&](const std::shared_ptr<ServerSession>& session) {
				return session->requestText(
					currentUser(), request, command, out, progress, errMsg);
			});
	}

	bool requestBinary(const std::string& serverId,
					   const std::vector<uint8_t>& request,
					   COMMAND command,
					   std::vector<uint8_t>& out,
					   std::atomic_uint64_t* progress = nullptr,
					   std::string* errMsg = nullptr,
					   search_protocol::ServerErrorInfo* serverError = nullptr)
	{
		if (serverError)
			serverError->reset();
		return runPreparedRequest(serverId, errMsg,
			[&](const std::shared_ptr<ServerSession>& session) {
				return session->requestBinary(
					currentUser(), request, command, out, progress, errMsg, serverError);
			});
	}

	bool requestBinFile(const std::string& serverId,
						const std::string& path,
						std::vector<uint8_t>& out,
						std::atomic_uint64_t* progress = nullptr,
						std::string* errMsg = nullptr)
	{
		return runPreparedRequest(serverId, errMsg,
			[&](const std::shared_ptr<ServerSession>& session) {
				return session->requestBinFile(
					currentUser(), path, out, progress, errMsg);
			});
	}

	bool requestFileToPath(const std::string& serverId,
						   const std::string& request,
						   COMMAND command,
						   const std::string& outPath,
						   MyAsioClient::FileProgressCallback onProgress = nullptr,
						   std::uint64_t* receivedSize = nullptr,
						   std::string* errMsg = nullptr,
						   bool flushStreamToOs = false,
						   bool replaceAfterSuccess = false)
	{
		return runPreparedRequest(serverId, errMsg,
			[&](const std::shared_ptr<ServerSession>& session) {
				return session->requestFileToPath(
					currentUser(), request, command, outPath, onProgress, receivedSize,
					errMsg, flushStreamToOs, replaceAfterSuccess);
			});
	}

	bool uploadFileV1(const std::string& serverId,
					  const std::wstring& localPath,
					  COMMAND command,
					  std::string* savedName = nullptr,
					  std::string* errMsg = nullptr,
					  search_protocol::ServerErrorInfo* serverError = nullptr)
	{
		if (serverError)
			serverError->reset();
		return runPreparedRequest(serverId, errMsg,
			[&](const std::shared_ptr<ServerSession>& session) {
				return session->uploadFileV1(
					currentUser(), localPath, command, savedName, errMsg, serverError);
			});
	}

private:
	struct DeviceAuthCache
	{
		std::string clientName;
		std::string clientId;
		std::string deviceType;
		std::string deviceId;
		std::string signature;
		bool present{false};
	};

	void rememberDeviceAuth(const std::string& clientName,
							const std::string& clientId,
							const std::string& deviceType,
							const std::string& deviceId,
							const std::string& signature)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		deviceAuthCache_.clientName = clientName;
		deviceAuthCache_.clientId = clientId;
		deviceAuthCache_.deviceType = deviceType;
		deviceAuthCache_.deviceId = deviceId;
		deviceAuthCache_.signature = signature;
		deviceAuthCache_.present = true;
	}

	bool ensureDeviceSession(const std::string& serverId, std::string* errMsg)
	{
		if (authPolicy() != ServerAuthPolicy::AuthenticateV1)
			return true;
		auto session = findSession(serverId);
		if (!session)
			return missingServer(serverId, errMsg);
		if (session->isConnected() && session->isServerAuthorized())
			return true;
		DeviceAuthCache creds;
		{
			std::lock_guard<std::mutex> lock(mutex_);
			creds = deviceAuthCache_;
		}
		if (!creds.present)
			return true;
		return authenticateV1(
			serverId,
			creds.clientName,
			creds.clientId,
			creds.deviceType,
			creds.deviceId,
			creds.signature,
			errMsg);
	}

	template<typename Fn>
	bool runPreparedRequest(const std::string& serverId, std::string* errMsg, Fn&& fn)
	{
		auto session = findSession(serverId);
		if (!session)
			return missingServer(serverId, errMsg);
		if (!ensureDeviceSession(serverId, errMsg))
			return false;
		if (fn(session))
			return true;
		if (authPolicy() != ServerAuthPolicy::AuthenticateV1)
			return false;
		if (session->isConnected() && session->isServerAuthorized())
			return false;
		if (!ensureDeviceSession(serverId, errMsg))
			return false;
		return fn(session);
	}
	std::shared_ptr<ServerSession> findSession(const std::string& serverId) const
	{
		std::lock_guard<std::mutex> lock(mutex_);
		auto it = sessions_.find(serverId);
		return it == sessions_.end() ? std::shared_ptr<ServerSession>{} : it->second;
	}

	std::string currentUser() const
	{
		std::lock_guard<std::mutex> lock(mutex_);
		return user_;
	}

	static bool missingServer(const std::string& serverId, std::string* errMsg)
	{
		if (errMsg) *errMsg = "Неизвестный идентификатор сервера: " + serverId;
		return false;
	}

	mutable std::mutex mutex_;
	std::map<std::string, std::shared_ptr<ServerSession>> sessions_;
	std::vector<std::string> order_;
	std::string primaryServerId_;
	std::string user_;
	ServerAuthPolicy authPolicy_{ServerAuthPolicy::LegacyUserRegistry};
	DeviceAuthCache deviceAuthCache_{};
};
