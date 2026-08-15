#pragma once

#include "myAsioClient.h"

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
};

class ServerSession
{
public:
	explicit ServerSession(ServerEndpoint endpoint)
		: endpoint_(std::move(endpoint))
	{
	}

	const ServerEndpoint& endpoint() const noexcept { return endpoint_; }

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
		return ensureConnectedLocked(user, errMsg);
	}

	void disconnect()
	{
		std::lock_guard<std::mutex> lock(requestMutex_);
		client_.disConnect();
		registeredUser_.clear();
	}

	bool isConnected()
	{
		std::lock_guard<std::mutex> lock(requestMutex_);
		return client_.isConnected();
	}

	bool requestText(const std::string& user,
					 const std::string& request,
					 COMMAND command,
					 std::string& out,
					 std::atomic_uint64_t* progress = nullptr,
					 std::string* errMsg = nullptr)
	{
		std::lock_guard<std::mutex> lock(requestMutex_);
		if (!ensureConnectedLocked(user, errMsg))
			return false;
		const bool ok = client_.requestText(request, command, out, progress, errMsg);
		if (!ok && !client_.isConnected())
			registeredUser_.clear();
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
		if (!ensureConnectedLocked(user, errMsg))
			return false;
		const bool ok = client_.requestBinary(
			request, command, out, progress, errMsg, serverError);
		if (!ok && !client_.isConnected())
			registeredUser_.clear();
		return ok;
	}

	bool requestBinFile(const std::string& user,
						const std::string& path,
						std::vector<uint8_t>& out,
						std::atomic_uint64_t* progress = nullptr,
						std::string* errMsg = nullptr)
	{
		std::lock_guard<std::mutex> lock(requestMutex_);
		if (!ensureConnectedLocked(user, errMsg))
			return false;
		const bool ok = client_.requestBinFile(path, out, progress, errMsg);
		if (!ok && !client_.isConnected())
			registeredUser_.clear();
		return ok;
	}

	bool requestFileToPath(const std::string& user,
						   const std::string& request,
						   COMMAND command,
						   const std::string& outPath,
						   MyAsioClient::FileProgressCallback onProgress = nullptr,
						   std::uint64_t* receivedSize = nullptr,
						   std::string* errMsg = nullptr,
						   bool flushStreamToOs = false)
	{
		std::lock_guard<std::mutex> lock(requestMutex_);
		if (!ensureConnectedLocked(user, errMsg))
			return false;
		const bool ok = client_.requestFileToPath(
			request, command, outPath, onProgress, receivedSize, errMsg, flushStreamToOs);
		if (!ok && !client_.isConnected())
			registeredUser_.clear();
		return ok;
	}

private:
	enum class ProtocolSupport
	{
		Unknown,
		Legacy,
		TypedErrorsV1
	};

	bool ensureConnectedLocked(const std::string& user, std::string* errMsg)
	{
		if (!client_.isConnected()) {
			std::string connectError;
			if (!client_.tryConnect(endpoint_.host, endpoint_.port, connectError)) {
				if (errMsg) *errMsg = connectError;
				return false;
			}
			registeredUser_.clear();

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

		if (!user.empty() && registeredUser_ != user) {
			std::string reply;
			std::string registryError;
			if (!client_.requestText(user, COMMAND::USER_REGISTRY, reply, nullptr, &registryError)) {
				if (errMsg) *errMsg = registryError;
				return false;
			}
			if (reply != "OK") {
				if (errMsg) *errMsg = "Сервер отклонил регистрацию пользователя: " + reply;
				return false;
			}
			registeredUser_ = user;
		}
		return true;
	}

	ServerEndpoint endpoint_;
	MyAsioClient client_;
	std::mutex requestMutex_;
	std::string registeredUser_;
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
			sessions[endpoint.id] = std::make_shared<ServerSession>(endpoint);
			order.push_back(endpoint.id);
		}

		std::lock_guard<std::mutex> lock(mutex_);
		sessions_.swap(sessions);
		order_.swap(order);
		primaryServerId_ = primaryServerId;
		if (sessions_.find(primaryServerId_) == sessions_.end())
			primaryServerId_ = order_.empty() ? std::string{} : order_.front();
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

	bool reconnect(const std::string& serverId, std::string* errMsg = nullptr)
	{
		auto session = findSession(serverId);
		if (!session) return missingServer(serverId, errMsg);
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
		auto session = findSession(serverId);
		if (!session) return missingServer(serverId, errMsg);
		return session->requestText(currentUser(), request, command, out, progress, errMsg);
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
		auto session = findSession(serverId);
		if (!session) return missingServer(serverId, errMsg);
		return session->requestBinary(
			currentUser(), request, command, out, progress, errMsg, serverError);
	}

	bool requestBinFile(const std::string& serverId,
						const std::string& path,
						std::vector<uint8_t>& out,
						std::atomic_uint64_t* progress = nullptr,
						std::string* errMsg = nullptr)
	{
		auto session = findSession(serverId);
		if (!session) return missingServer(serverId, errMsg);
		return session->requestBinFile(currentUser(), path, out, progress, errMsg);
	}

	bool requestFileToPath(const std::string& serverId,
						   const std::string& request,
						   COMMAND command,
						   const std::string& outPath,
						   MyAsioClient::FileProgressCallback onProgress = nullptr,
						   std::uint64_t* receivedSize = nullptr,
						   std::string* errMsg = nullptr,
						   bool flushStreamToOs = false)
	{
		auto session = findSession(serverId);
		if (!session) return missingServer(serverId, errMsg);
		return session->requestFileToPath(
			currentUser(), request, command, outPath, onProgress, receivedSize, errMsg,
			flushStreamToOs);
	}

private:
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
};
