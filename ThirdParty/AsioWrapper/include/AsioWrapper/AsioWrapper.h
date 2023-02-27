#pragma once

struct AsioContent {
	void Release();
	virtual ~AsioContent() = delete;
};

struct AsioIpSocket {
	virtual ~AsioIpSocket() = delete;
};