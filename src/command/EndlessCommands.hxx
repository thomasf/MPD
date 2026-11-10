// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "CommandResult.hxx"
#include "Request.hxx"
#include "client/Response.hxx"
#include "Partition.hxx"
#include "client/Client.hxx"
#include "decoder/EndlessMode.hxx"
#include "protocol/IdleFlags.hxx"

inline CommandResult
handle_endless(Client &client, Request args, [[maybe_unused]] Response &r)
{
	bool new_value;
	if (args.empty()) {
		new_value = !global_endless_mode.load(std::memory_order_relaxed);
	} else {
		new_value = args.ParseBool(0);
	}

	if (new_value != global_endless_mode.exchange(new_value, std::memory_order_relaxed)) {
		client.GetPartition().EmitIdle(IDLE_OPTIONS);
	}

	return CommandResult::OK;
}
