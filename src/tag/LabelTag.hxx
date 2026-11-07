// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef MPD_TAG_LABEL_TAG_HXX
#define MPD_TAG_LABEL_TAG_HXX

#include "Handler.hxx"

#include <array>
#include <span>
#include <string>
#include <string_view>
#include <vector>

/**
 * A #TagHandler wrapper which chooses the record label (#TAG_LABEL)
 * from several source keys, because files in the wild store it under
 * different keys.
 *
 * Candidates are taken from OnPair() (matching keys
 * case-insensitively against the priority list) and from OnTag()
 * with #TAG_LABEL (the format's native mapping, see #NATIVE).  Only
 * the non-empty values of the highest-priority source that has one
 * are passed to the wrapped handler, in the destructor.
 *
 * To use it in a scanner, wrap the #TagHandler at the scanner's entry
 * point:
 *
 *     void Scan(..., TagHandler &_handler) noexcept
 *     {
 *             LabelTagHandler handler{_handler};
 *             ...
 */
class LabelTagHandler final : public TagHandler {
public:
	/**
	 * In a priority list, this stands for the format's native
	 * #TAG_LABEL mapping (i.e. OnTag() calls).  If it is not in
	 * the list, those calls are ignored.
	 */
	static constexpr std::string_view NATIVE{};

	/**
	 * For formats with free-form keys: Vorbis comments (FLAC,
	 * Ogg, Opus), APE and FFmpeg metadata.
	 */
	static constexpr std::array DEFAULT_PRIORITY{
		std::string_view{"label"},
		std::string_view{"publisher"},
		std::string_view{"organization"},
	};

	/**
	 * For ID3v2: the native mapping is "TPUB", and "TXXX" frames
	 * are pairs.
	 */
	static constexpr std::array ID3_PRIORITY{
		std::string_view{"label"},
		NATIVE,
		std::string_view{"organization"},
	};

private:
	TagHandler &next;
	const std::span<const std::string_view> priority;

	std::size_t best_rank;
	std::vector<std::string> values;

public:
	explicit LabelTagHandler(TagHandler &_next,
				 std::span<const std::string_view> _priority=DEFAULT_PRIORITY) noexcept;

	/**
	 * Submits the chosen label values to the wrapped handler.
	 */
	~LabelTagHandler() noexcept;

	void OnDuration(SongTime duration) noexcept override;
	void OnTag(TagType type, std::string_view value) noexcept override;
	void OnPair(std::string_view key,
		    std::string_view value) noexcept override;
	void OnAudioFormat(AudioFormat af) noexcept override;
	void OnPicture(std::string_view mime_type,
		       std::span<const std::byte> buffer) noexcept override;

private:
	void Add(std::size_t rank, std::string_view value) noexcept;
};

#endif
