// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "tag/LabelTag.hxx"
#include "tag/Builder.hxx"
#include "tag/Handler.hxx"
#include "tag/Tag.hxx"

#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <vector>

using V = std::vector<std::string>;

/**
 * A pair with an empty key stands for the format's native
 * #TAG_LABEL mapping, i.e. an OnTag() call.
 */
using Items = std::initializer_list<std::pair<std::string_view, std::string_view>>;

static V
Scan(Items items,
     std::span<const std::string_view> priority=LabelTagHandler::DEFAULT_PRIORITY)
{
	TagBuilder builder;
	AddTagHandler next{builder};

	{
		LabelTagHandler handler{next, priority};
		EXPECT_TRUE(handler.WantPair());

		for (const auto &[key, value] : items) {
			if (key.empty())
				handler.OnTag(TAG_LABEL, value);
			else
				handler.OnPair(key, value);
		}

		handler.OnTag(TAG_ARTIST, "artist");
	}

	const Tag tag = builder.Commit();
	EXPECT_STREQ(tag.GetValue(TAG_ARTIST), "artist");

	V result;
	for (const auto &item : tag)
		if (item.type == TAG_LABEL)
			result.emplace_back(item.value);
	return result;
}

TEST(LabelTag, Empty)
{
	EXPECT_EQ(Scan({}), V{});
	EXPECT_EQ(Scan({{"ARTIST", "x"}}), V{});
}

TEST(LabelTag, Priority)
{
	/* order of appearance does not matter */
	EXPECT_EQ(Scan({{"ORGANIZATION", "o"}, {"PUBLISHER", "p"}, {"LABEL", "l"}}),
		  V{"l"});
	EXPECT_EQ(Scan({{"LABEL", "l"}, {"PUBLISHER", "p"}}), V{"l"});
	EXPECT_EQ(Scan({{"ORGANIZATION", "o"}, {"Publisher", "p"}}), V{"p"});
	EXPECT_EQ(Scan({{"organization", "o"}}), V{"o"});
}

TEST(LabelTag, MultipleValues)
{
	EXPECT_EQ(Scan({{"PUBLISHER", "p1"}, {"LABEL", "l1"},
			{"ORGANIZATION", "o"}, {"LABEL", "l2"}}),
		  (V{"l1", "l2"}));
}

TEST(LabelTag, EmptyValuesAreIgnored)
{
	EXPECT_EQ(Scan({{"LABEL", ""}, {"PUBLISHER", "p"}}), V{"p"});
	EXPECT_EQ(Scan({{"PUBLISHER", "p"}, {"LABEL", ""}}), V{"p"});
}

TEST(LabelTag, NativeIgnoredByDefault)
{
	/* the native mapping duplicates the "label" key */
	EXPECT_EQ(Scan({{"", "l"}, {"LABEL", "l"}}), V{"l"});
	EXPECT_EQ(Scan({{"", "x"}}), V{});
}

TEST(LabelTag, Id3)
{
	const auto &id3 = LabelTagHandler::ID3_PRIORITY;

	EXPECT_EQ(Scan({{"ORGANIZATION", "o"}, {"", "tpub"}, {"LABEL", "l"}}, id3),
		  V{"l"});
	EXPECT_EQ(Scan({{"ORGANIZATION", "o"}, {"", "tpub"}}, id3), V{"tpub"});
	EXPECT_EQ(Scan({{"ORGANIZATION", "o"}, {"", ""}}, id3), V{"o"});
	EXPECT_EQ(Scan({{"PUBLISHER", "p"}}, id3), V{});
}

TEST(LabelTag, PairsForwardedOnlyIfWanted)
{
	struct PairCounter final : NullTagHandler {
		unsigned pairs = 0;
		PairCounter(unsigned mask) noexcept:NullTagHandler(mask) {}
		void OnPair(std::string_view, std::string_view) noexcept override {
			++pairs;
		}
	};

	PairCounter tags_only{TagHandler::WANT_TAG};
	{
		LabelTagHandler handler{tags_only};
		handler.OnPair("LABEL", "l");
	}
	EXPECT_EQ(tags_only.pairs, 0u);

	PairCounter with_pairs{TagHandler::WANT_TAG|TagHandler::WANT_PAIR};
	{
		LabelTagHandler handler{with_pairs};
		handler.OnPair("LABEL", "l");
	}
	EXPECT_EQ(with_pairs.pairs, 1u);

	PairCounter nothing{0};
	EXPECT_FALSE(LabelTagHandler{nothing}.WantPair());
}
