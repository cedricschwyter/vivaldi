// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/common/interest_group/auction_config_mojom_traits.h"

#include <string>
#include <tuple>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/time/time.h"
#include "base/unguessable_token.h"
#include "mojo/public/cpp/test_support/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/blink/public/common/interest_group/auction_config.h"
#include "third_party/blink/public/mojom/interest_group/interest_group_types.mojom.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace blink {

bool operator==(const DirectFromSellerSignalsSubresource& a,
                const DirectFromSellerSignalsSubresource& b) {
  return std::tie(a.bundle_url, a.token) == std::tie(b.bundle_url, b.token);
}

bool operator==(const DirectFromSellerSignals& a,
                const DirectFromSellerSignals& b) {
  return std::tie(a.prefix, a.per_buyer_signals, a.seller_signals,
                  a.auction_signals) == std::tie(b.prefix, b.per_buyer_signals,
                                                 b.seller_signals,
                                                 b.auction_signals);
}

bool operator==(const AuctionConfig& a, const AuctionConfig& b);

bool operator==(const AuctionConfig::NonSharedParams& a,
                const AuctionConfig::NonSharedParams& b) {
  return std::tie(a.interest_group_buyers, a.auction_signals, a.seller_signals,
                  a.seller_timeout, a.per_buyer_signals, a.per_buyer_timeouts,
                  a.all_buyers_timeout, a.per_buyer_group_limits,
                  a.all_buyers_group_limit, a.per_buyer_priority_signals,
                  a.all_buyers_priority_signals, a.component_auctions) ==
         std::tie(b.interest_group_buyers, b.auction_signals, b.seller_signals,
                  b.seller_timeout, b.per_buyer_signals, b.per_buyer_timeouts,
                  b.all_buyers_timeout, b.per_buyer_group_limits,
                  b.all_buyers_group_limit, b.per_buyer_priority_signals,
                  b.all_buyers_priority_signals, b.component_auctions);
}

bool operator==(const AuctionConfig& a, const AuctionConfig& b) {
  return std::tie(a.seller, a.decision_logic_url, a.trusted_scoring_signals_url,
                  a.non_shared_params, a.direct_from_seller_signals,
                  a.seller_experiment_group_id, a.all_buyer_experiment_group_id,
                  a.per_buyer_experiment_group_ids) ==
         std::tie(b.seller, b.decision_logic_url, b.trusted_scoring_signals_url,
                  b.non_shared_params, b.direct_from_seller_signals,
                  b.seller_experiment_group_id, b.all_buyer_experiment_group_id,
                  b.per_buyer_experiment_group_ids);
}

namespace {

constexpr char kSellerOriginStr[] = "https://seller.test";

// Cases for direct_from_seller_signals test parameterization.

constexpr char kPerBuyerSignals[] = "per-buyer-signals";
constexpr char kSellerSignals[] = "seller-signals";
constexpr char kAuctionSignals[] = "auction-signals";

constexpr char kBundleUrl[] = "bundle-url";
constexpr char kPrefix[] = "prefix";

// Creates a minimal valid AuctionConfig, with a seller and the passed in
// decision logic URL. Seller is derived from `decision_logic_url`.
AuctionConfig CreateBasicConfig(
    const GURL& decision_logic_url = GURL("https://seller.test/foo")) {
  AuctionConfig auction_config;
  auction_config.seller = url::Origin::Create(decision_logic_url);
  auction_config.decision_logic_url = decision_logic_url;
  return auction_config;
}

// Creates an AuctionConfig with all fields except `component_auctions`
// populated.
AuctionConfig CreateFullConfig() {
  const url::Origin seller = url::Origin::Create(GURL(kSellerOriginStr));
  AuctionConfig auction_config = CreateBasicConfig();

  auction_config.trusted_scoring_signals_url = GURL("https://seller.test/bar");
  auction_config.seller_experiment_group_id = 1;
  auction_config.all_buyer_experiment_group_id = 2;

  const url::Origin buyer = url::Origin::Create(GURL("https://buyer.test"));
  auction_config.per_buyer_experiment_group_ids[buyer] = 3;

  AuctionConfig::NonSharedParams& non_shared_params =
      auction_config.non_shared_params;
  non_shared_params.interest_group_buyers.emplace();
  non_shared_params.interest_group_buyers->push_back(buyer);
  non_shared_params.auction_signals = "[4]";
  non_shared_params.seller_signals = "[5]";
  non_shared_params.seller_timeout = base::Seconds(6);
  non_shared_params.per_buyer_signals.emplace();
  (*non_shared_params.per_buyer_signals)[buyer] = "[7]";
  non_shared_params.per_buyer_timeouts.emplace();
  (*non_shared_params.per_buyer_timeouts)[buyer] = base::Seconds(8);
  non_shared_params.all_buyers_timeout = base::Seconds(9);
  non_shared_params.per_buyer_group_limits[buyer] = 10;
  non_shared_params.all_buyers_group_limit = 11;
  non_shared_params.per_buyer_priority_signals.emplace();
  (*non_shared_params.per_buyer_priority_signals)[buyer] = {
      {"hats", 1.5}, {"for", 0}, {"sale", -2}};
  non_shared_params.all_buyers_priority_signals = {
      {"goats", -1.5}, {"for", 5}, {"sale", 0}};

  DirectFromSellerSignalsSubresource
      direct_from_seller_signals_per_buyer_signals_buyer;
  direct_from_seller_signals_per_buyer_signals_buyer.bundle_url =
      GURL("https://seller.test/bundle");
  direct_from_seller_signals_per_buyer_signals_buyer.token =
      base::UnguessableToken::Create();

  DirectFromSellerSignalsSubresource direct_from_seller_seller_signals;
  direct_from_seller_seller_signals.bundle_url =
      GURL("https://seller.test/bundle");
  direct_from_seller_seller_signals.token = base::UnguessableToken::Create();

  DirectFromSellerSignalsSubresource direct_from_seller_auction_signals;
  direct_from_seller_auction_signals.bundle_url =
      GURL("https://seller.test/bundle");
  direct_from_seller_auction_signals.token = base::UnguessableToken::Create();

  DirectFromSellerSignals direct_from_seller_signals;
  direct_from_seller_signals.prefix = GURL("https://seller.test/json");
  direct_from_seller_signals.per_buyer_signals.insert(
      {buyer, std::move(direct_from_seller_signals_per_buyer_signals_buyer)});
  direct_from_seller_signals.seller_signals =
      std::move(direct_from_seller_seller_signals);
  direct_from_seller_signals.auction_signals =
      std::move(direct_from_seller_auction_signals);

  auction_config.direct_from_seller_signals =
      std::move(direct_from_seller_signals);

  return auction_config;
}

// Attempts to serialize and then deserialize `auction_config`, returning true
// if deserialization succeeded. On success, also checks that the resulting
// config matches the original config.
bool SerializeAndDeserialize(const AuctionConfig& auction_config) {
  AuctionConfig auction_config_clone;
  bool success =
      mojo::test::SerializeAndDeserialize<blink::mojom::AuctionAdConfig>(
          auction_config, auction_config_clone);

  if (success) {
    EXPECT_EQ(auction_config, auction_config_clone);
    // This *should* be implied by the above, but let's check...
    EXPECT_EQ(auction_config.non_shared_params,
              auction_config_clone.non_shared_params);
  }
  return success;
}

TEST(AuctionConfigMojomTraitsTest, Empty) {
  AuctionConfig auction_config;
  EXPECT_FALSE(SerializeAndDeserialize(auction_config));
}

TEST(AuctionConfigMojomTraitsTest, Basic) {
  AuctionConfig auction_config = CreateBasicConfig();
  EXPECT_TRUE(SerializeAndDeserialize(auction_config));
}

TEST(AuctionConfigMojomTraitsTest, SellerNotHttps) {
  AuctionConfig auction_config = CreateBasicConfig(GURL("http://seller.test"));
  EXPECT_FALSE(SerializeAndDeserialize(auction_config));
}

TEST(AuctionConfigMojomTraitsTest, SellerDecisionUrlMismatch) {
  AuctionConfig auction_config = CreateBasicConfig(GURL("http://seller.test"));
  // Different origin than seller, but same scheme.
  auction_config.decision_logic_url = GURL("https://not.seller.test/foo");
  EXPECT_FALSE(SerializeAndDeserialize(auction_config));

  auction_config = CreateBasicConfig(GURL("https://seller.test"));
  // This blob URL should be considered same-origin to the seller, but the
  // scheme is wrong.
  auction_config.decision_logic_url = GURL("blob:https://seller.test/foo");
  ASSERT_EQ(auction_config.seller,
            url::Origin::Create(auction_config.decision_logic_url));
  EXPECT_FALSE(SerializeAndDeserialize(auction_config));
}

TEST(AuctionConfigMojomTraitsTest, SellerScoringSignalsUrlMismatch) {
  AuctionConfig auction_config = CreateBasicConfig(GURL("http://seller.test"));
  // Different origin than seller, but same scheme.
  auction_config.trusted_scoring_signals_url =
      GURL("https://not.seller.test/foo");
  EXPECT_FALSE(SerializeAndDeserialize(auction_config));

  auction_config = CreateBasicConfig(GURL("https://seller.test"));
  // This blob URL should be considered same-origin to the seller, but the
  // scheme is wrong.
  auction_config.trusted_scoring_signals_url =
      GURL("blob:https://seller.test/foo");
  ASSERT_EQ(auction_config.seller,
            url::Origin::Create(*auction_config.trusted_scoring_signals_url));
  EXPECT_FALSE(SerializeAndDeserialize(auction_config));
}

TEST(AuctionConfigMojomTraitsTest, FullConfig) {
  AuctionConfig auction_config = CreateFullConfig();
  EXPECT_TRUE(SerializeAndDeserialize(auction_config));
}

TEST(AuctionConfigMojomTraitsTest,
     perBuyerPrioritySignalsCannotOverrideBrowserSignals) {
  const url::Origin kBuyer = url::Origin::Create(GURL("https://buyer.test"));

  AuctionConfig auction_config = CreateBasicConfig();
  auction_config.non_shared_params.interest_group_buyers.emplace();
  auction_config.non_shared_params.interest_group_buyers->push_back(kBuyer);
  auction_config.non_shared_params.per_buyer_priority_signals.emplace();
  (*auction_config.non_shared_params.per_buyer_priority_signals)[kBuyer] = {
      {"browserSignals.hats", 1}};

  EXPECT_FALSE(SerializeAndDeserialize(auction_config));
}

TEST(AuctionConfigMojomTraitsTest,
     allBuyersPrioritySignalsCannotOverrideBrowserSignals) {
  AuctionConfig auction_config = CreateBasicConfig();
  auction_config.non_shared_params.all_buyers_priority_signals = {
      {"browserSignals.goats", 2}};
  EXPECT_FALSE(SerializeAndDeserialize(auction_config));
}

TEST(AuctionConfigMojomTraitsTest, BuyerNotHttps) {
  AuctionConfig auction_config = CreateBasicConfig();
  auction_config.non_shared_params.interest_group_buyers.emplace();
  auction_config.non_shared_params.interest_group_buyers->push_back(
      url::Origin::Create(GURL("http://buyer.test")));
  EXPECT_FALSE(SerializeAndDeserialize(auction_config));
}

TEST(AuctionConfigMojomTraitsTest, BuyerNotHttpsMultipleBuyers) {
  AuctionConfig auction_config = CreateBasicConfig();
  auction_config.non_shared_params.interest_group_buyers.emplace();
  auction_config.non_shared_params.interest_group_buyers->push_back(
      url::Origin::Create(GURL("https://buyer1.test")));
  auction_config.non_shared_params.interest_group_buyers->push_back(
      url::Origin::Create(GURL("http://buyer2.test")));
  EXPECT_FALSE(SerializeAndDeserialize(auction_config));
}

TEST(AuctionConfigMojomTraitsTest, ComponentAuctionUrlHttps) {
  AuctionConfig auction_config = CreateBasicConfig();
  auction_config.non_shared_params.component_auctions.emplace_back(
      CreateBasicConfig(GURL("http://seller.test")));
  EXPECT_FALSE(SerializeAndDeserialize(auction_config));
}

TEST(AuctionConfigMojomTraitsTest, ComponentAuctionTooDeep) {
  AuctionConfig auction_config = CreateBasicConfig();
  auction_config.non_shared_params.component_auctions.emplace_back(
      CreateBasicConfig());
  auction_config.non_shared_params.component_auctions[0]
      .non_shared_params.component_auctions.emplace_back(CreateBasicConfig());
  EXPECT_FALSE(SerializeAndDeserialize(auction_config));
}

TEST(AuctionConfigMojomTraitsTest, ComponentAuctionSuccessSingleBasic) {
  AuctionConfig auction_config = CreateBasicConfig();
  auction_config.non_shared_params.component_auctions.emplace_back(
      CreateBasicConfig());
  EXPECT_TRUE(SerializeAndDeserialize(auction_config));
}

TEST(AuctionConfigMojomTraitsTest, ComponentAuctionSuccessMultipleFull) {
  AuctionConfig auction_config = CreateFullConfig();
  auction_config.non_shared_params.component_auctions.emplace_back(
      CreateFullConfig());
  auction_config.non_shared_params.component_auctions.emplace_back(
      CreateFullConfig());
  EXPECT_TRUE(SerializeAndDeserialize(auction_config));
}

TEST(AuctionConfigMojomTraitsTest,
     DirectFromSellerSignalsPrefixWithQueryString) {
  AuctionConfig auction_config = CreateFullConfig();
  auction_config.direct_from_seller_signals->prefix =
      GURL("https://seller.test/json?queryPart");
  EXPECT_FALSE(SerializeAndDeserialize(auction_config));
}

TEST(AuctionConfigMojomTraitsTest, DirectFromSellerSignalsBuyerNotPresent) {
  AuctionConfig auction_config = CreateFullConfig();
  DirectFromSellerSignalsSubresource& buyer2_subresource =
      auction_config.direct_from_seller_signals
          ->per_buyer_signals[url::Origin::Create(GURL("https://buyer2.test"))];
  buyer2_subresource.bundle_url = GURL("https://seller.test/bundle");
  buyer2_subresource.token = base::UnguessableToken::Create();
  EXPECT_FALSE(SerializeAndDeserialize(auction_config));
}

TEST(AuctionConfigMojomTraitsTest,
     DirectFromSellerSignalsNoDirectFromSellerSignals) {
  AuctionConfig auction_config = CreateFullConfig();
  auction_config.direct_from_seller_signals = absl::nullopt;
  EXPECT_TRUE(SerializeAndDeserialize(auction_config));
}

TEST(AuctionConfigMojomTraitsTest, DirectFromSellerSignalsNoPerBuyerSignals) {
  AuctionConfig auction_config = CreateFullConfig();
  auction_config.direct_from_seller_signals->per_buyer_signals.clear();
  EXPECT_TRUE(SerializeAndDeserialize(auction_config));
}

TEST(AuctionConfigMojomTraitsTest, DirectFromSellerSignalsNoSellerSignals) {
  AuctionConfig auction_config = CreateFullConfig();
  auction_config.direct_from_seller_signals->seller_signals = absl::nullopt;
  EXPECT_TRUE(SerializeAndDeserialize(auction_config));
}

TEST(AuctionConfigMojomTraitsTest, DirectFromSellerSignalsNoAuctionSignals) {
  AuctionConfig auction_config = CreateFullConfig();
  auction_config.direct_from_seller_signals->auction_signals = absl::nullopt;
  EXPECT_TRUE(SerializeAndDeserialize(auction_config));
}

class AuctionConfigMojomTraitsDirectFromSellerSignalsTest
    : public ::testing::TestWithParam<std::tuple<const char*, const char*>> {
 public:
  GURL& GetMutableURL(AuctionConfig& auction_config) const {
    const std::string which_path = WhichPath();
    if (which_path == kPrefix) {
      return auction_config.direct_from_seller_signals->prefix;
    } else {
      EXPECT_EQ(which_path, kBundleUrl);
      const std::string which_bundle = WhichBundle();
      if (which_bundle == kPerBuyerSignals) {
        return auction_config.direct_from_seller_signals->per_buyer_signals
            .at(url::Origin::Create(GURL("https://buyer.test")))
            .bundle_url;
      } else if (which_bundle == kSellerSignals) {
        return auction_config.direct_from_seller_signals->seller_signals
            ->bundle_url;
      } else {
        EXPECT_EQ(which_bundle, kAuctionSignals);
        return auction_config.direct_from_seller_signals->auction_signals
            ->bundle_url;
      }
    }
  }

  std::string GetURLPath() const {
    const std::string which_path = WhichPath();
    if (which_path == kBundleUrl) {
      return "/bundle";
    } else {
      EXPECT_EQ(which_path, kPrefix);
      return "/json";
    }
  }

 private:
  std::string WhichBundle() const { return std::get<0>(GetParam()); }
  std::string WhichPath() const { return std::get<1>(GetParam()); }
};

TEST_P(AuctionConfigMojomTraitsDirectFromSellerSignalsTest, NotHttps) {
  AuctionConfig auction_config = CreateFullConfig();
  GetMutableURL(auction_config) = GURL("http://seller.test" + GetURLPath());
  EXPECT_FALSE(SerializeAndDeserialize(auction_config));
}

TEST_P(AuctionConfigMojomTraitsDirectFromSellerSignalsTest, WrongOrigin) {
  AuctionConfig auction_config = CreateFullConfig();
  GetMutableURL(auction_config) = GURL("https://seller2.test" + GetURLPath());
  EXPECT_FALSE(SerializeAndDeserialize(auction_config));
}

INSTANTIATE_TEST_SUITE_P(All,
                         AuctionConfigMojomTraitsDirectFromSellerSignalsTest,
                         ::testing::Combine(::testing::Values(kPerBuyerSignals,
                                                              kSellerSignals,
                                                              kAuctionSignals),
                                            ::testing::Values(kBundleUrl,
                                                              kPrefix)));

}  // namespace

}  // namespace blink
