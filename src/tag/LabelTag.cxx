// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "LabelTag.hxx"
#include "Type.hxx"
#include "pcm/AudioFormat.hxx"
#include "util/StringCompare.hxx"

#include <algorithm>

/**
 * Wants everything the wrapped handler wants, plus pairs if it wants
 * tags (because pairs are label candidates).
 */
static unsigned
GetWantMask(const TagHandler &h) noexcept
{
	return (h.WantDuration() ? TagHandler::WANT_DURATION : 0) |
		(h.WantTag() ? TagHandler::WANT_TAG | TagHandler::WANT_PAIR : 0) |
		(h.WantPair() ? TagHandler::WANT_PAIR : 0) |
		(h.WantAudioFormat() ? TagHandler::WANT_AUDIO_FORMAT : 0) |
		(h.WantPicture() ? TagHandler::WANT_PICTURE : 0);
}

LabelTagHandler::LabelTagHandler(TagHandler &_next,
				 std::span<const std::string_view> _priority) noexcept
	:TagHandler(GetWantMask(_next)),
	 next(_next), priority(_priority),
	 best_rank(priority.size()) {}

LabelTagHandler::~LabelTagHandler() noexcept
{
	for (const auto &value : values)
		next.OnTag(TAG_LABEL, value);
}

void
LabelTagHandler::Add(std::size_t rank, std::string_view value) noexcept
{
	if (value.empty() || rank > best_rank)
		return;

	if (rank < best_rank) {
		best_rank = rank;
		values.clear();
	}

	values.emplace_back(value);
}

void
LabelTagHandler::OnDuration(SongTime duration) noexcept
{
	next.OnDuration(duration);
}

void
LabelTagHandler::OnTag(TagType type, std::string_view value) noexcept
{
	if (type != TAG_LABEL) {
		next.OnTag(type, value);
		return;
	}

	const auto i = std::ranges::find(priority, NATIVE);
	if (i != priority.end())
		Add(std::distance(priority.begin(), i), value);
}

void
LabelTagHandler::OnPair(std::string_view key, std::string_view value) noexcept
{
	if (next.WantPair())
		next.OnPair(key, value);

	if (key.empty() || !next.WantTag())
		return;

	const auto i = std::ranges::find_if(priority, [key](std::string_view name){
		return StringIsEqualIgnoreCase(key, name);
	});
	if (i != priority.end())
		Add(std::distance(priority.begin(), i), value);
}

void
LabelTagHandler::OnAudioFormat(AudioFormat af) noexcept
{
	next.OnAudioFormat(af);
}

void
LabelTagHandler::OnPicture(std::string_view mime_type,
			   std::span<const std::byte> buffer) noexcept
{
	next.OnPicture(mime_type, buffer);
}
