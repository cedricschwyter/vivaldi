// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/first_party_sets/first_party_sets_handler_impl.h"

#include <string>

#include "base/callback_helpers.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/json/json_reader.h"
#include "base/run_loop.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/version.h"
#include "content/browser/first_party_sets/first_party_set_parser.h"
#include "content/browser/first_party_sets/local_set_declaration.h"
#include "content/public/browser/first_party_sets_handler.h"
#include "content/public/common/content_features.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_browser_context.h"
#include "net/base/schemeful_site.h"
#include "net/first_party_sets/first_party_set_entry.h"
#include "net/first_party_sets/first_party_sets_cache_filter.h"
#include "net/first_party_sets/first_party_sets_context_config.h"
#include "net/first_party_sets/global_first_party_sets.h"
#include "testing/gmock/include/gmock/gmock-matchers.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

using ::testing::Eq;
using ::testing::IsEmpty;
using ::testing::Optional;
using ::testing::Pair;
using ::testing::SizeIs;
using ::testing::UnorderedElementsAre;

// Some of these tests overlap with FirstPartySetParser unittests, but
// overlapping test coverage isn't the worst thing.
namespace content {

namespace {

using ParseErrorType = FirstPartySetsHandler::ParseErrorType;
using ParseWarningType = FirstPartySetsHandler::ParseWarningType;

const char* kAdditionsField = "additions";
const char* kPrimaryField = "primary";
const char* kCctldsField = "ccTLDs";

const char* kFirstPartySetsClearSiteDataOutcomeHistogram =
    "FirstPartySets.Initialization.ClearSiteDataOutcomeType";

net::GlobalFirstPartySets GetSetsAndWait() {
  base::test::TestFuture<net::GlobalFirstPartySets> future;
  absl::optional<net::GlobalFirstPartySets> result =
      FirstPartySetsHandlerImpl::GetInstance()->GetSets(future.GetCallback());
  return result.has_value() ? std::move(result).value() : future.Take();
}

absl::optional<net::GlobalFirstPartySets> GetPersistedGlobalSetsAndWait(
    const std::string& browser_context_id) {
  base::test::TestFuture<absl::optional<net::GlobalFirstPartySets>> future;
  FirstPartySetsHandlerImpl::GetInstance()->GetPersistedGlobalSetsForTesting(
      browser_context_id, future.GetCallback());
  return future.Take();
}

absl::optional<bool> HasEntryInBrowserContextsClearedAndWait(
    const std::string& browser_context_id) {
  base::test::TestFuture<absl::optional<bool>> future;
  FirstPartySetsHandlerImpl::GetInstance()->HasBrowserContextClearedForTesting(
      browser_context_id, future.GetCallback());
  return future.Take();
}

}  // namespace

TEST(FirstPartySetsHandlerImpl, ValidateEnterprisePolicy_ValidPolicy) {
  base::Value input = base::JSONReader::Read(R"(
             {
                "replacements": [
                  {
                    "primary": "https://primary1.test",
                    "associatedSites": ["https://associatedsite1.test"]
                  }
                ],
                "additions": [
                  {
                    "primary": "https://primary2.test",
                    "associatedSites": ["https://associatedsite2.test"]
                  }
                ]
              }
            )")
                          .value();
  // Validation doesn't fail with an error and there are no warnings to output.
  std::pair<absl::optional<FirstPartySetsHandler::ParseError>,
            std::vector<FirstPartySetsHandler::ParseWarning>>
      opt_error_and_warnings =
          FirstPartySetsHandler::ValidateEnterprisePolicy(input.GetDict());
  EXPECT_FALSE(opt_error_and_warnings.first.has_value());
  EXPECT_THAT(opt_error_and_warnings.second, IsEmpty());
}

TEST(FirstPartySetsHandlerImpl,
     ValidateEnterprisePolicy_ValidPolicyWithWarnings) {
  // Some input that matches our policies schema but returns non-fatal warnings.
  base::Value input = base::JSONReader::Read(R"(
              {
                "replacements": [],
                "additions": [
                  {
                    "primary": "https://primary1.test",
                    "associatedSites": ["https://associatedsite1.test"],
                    "ccTLDs": {
                      "https://non-canonical.test": ["https://primary1.test"]
                    }
                  }
                ]
              }
            )")
                          .value();
  // Validation succeeds without errors.
  std::pair<absl::optional<FirstPartySetsHandler::ParseError>,
            std::vector<FirstPartySetsHandler::ParseWarning>>
      opt_error_and_warnings =
          FirstPartySetsHandler::ValidateEnterprisePolicy(input.GetDict());
  EXPECT_FALSE(opt_error_and_warnings.first.has_value());
  // Outputs metadata that can be used to surface a descriptive warning.
  EXPECT_EQ(opt_error_and_warnings.second,
            std::vector<FirstPartySetsHandler::ParseWarning>{
                FirstPartySetsHandler::ParseWarning(
                    ParseWarningType::kCctldKeyNotCanonical,
                    {kAdditionsField, 0, kCctldsField,
                     "https://non-canonical.test"})});
}

TEST(FirstPartySetsHandlerImpl, ValidateEnterprisePolicy_InvalidPolicy) {
  // Some input that matches our policies schema but breaks FPS invariants.
  // For more test coverage, see the ParseSetsFromEnterprisePolicy unit tests.
  base::Value input = base::JSONReader::Read(R"(
              {
                "replacements": [
                  {
                    "primary": "https://primary1.test",
                    "associatedSites": ["https://associatedsite1.test"]
                  }
                ],
                "additions": [
                  {
                    "primary": "https://primary1.test",
                    "associatedSites": ["https://associatedsite2.test"]
                  }
                ]
              }
            )")
                          .value();
  // Validation fails with an error.
  std::pair<absl::optional<FirstPartySetsHandler::ParseError>,
            std::vector<FirstPartySetsHandler::ParseWarning>>
      opt_error_and_warnings =
          FirstPartySetsHandler::ValidateEnterprisePolicy(input.GetDict());
  EXPECT_TRUE(opt_error_and_warnings.first.has_value());
  // An appropriate ParseError is returned.
  EXPECT_EQ(
      opt_error_and_warnings.first.value(),
      FirstPartySetsHandler::ParseError(ParseErrorType::kNonDisjointSets,
                                        {kAdditionsField, 0, kPrimaryField}));
}

class FirstPartySetsHandlerImplTest : public ::testing::Test {
 public:
  explicit FirstPartySetsHandlerImplTest(bool enabled) {
    FirstPartySetsHandlerImpl::GetInstance()->SetEnabledForTesting(enabled);

    CHECK(scoped_dir_.CreateUniqueTempDir());
    CHECK(PathExists(scoped_dir_.GetPath()));
  }

  base::File WritePublicSetsFile(base::StringPiece content) {
    base::FilePath path =
        scoped_dir_.GetPath().Append(FILE_PATH_LITERAL("sets_file.json"));
    CHECK(base::WriteFile(path, content));

    return base::File(path, base::File::FLAG_OPEN | base::File::FLAG_READ);
  }

  void TearDown() override {
    FirstPartySetsHandlerImpl::GetInstance()->ResetForTesting();
  }

  BrowserContext* context() { return &context_; }

 protected:
  base::ScopedTempDir scoped_dir_;
  BrowserTaskEnvironment env_;
  TestBrowserContext context_;
};

class FirstPartySetsHandlerImplEnabledTest
    : public FirstPartySetsHandlerImplTest {
 public:
  FirstPartySetsHandlerImplEnabledTest()
      : FirstPartySetsHandlerImplTest(true) {}
};

TEST_F(FirstPartySetsHandlerImplEnabledTest, EmptyDBPath) {
  net::SchemefulSite example(GURL("https://example.test"));
  net::SchemefulSite associated(GURL("https://associatedsite1.test"));

  // Empty `user_data_dir` will fail to load persisted sets, but that will not
  // prevent `on_sets_ready` from being invoked.
  FirstPartySetsHandlerImpl::GetInstance()->Init(
      /*user_data_dir=*/{},
      LocalSetDeclaration(
          R"({"primary": "https://example.test",)"
          R"("associatedSites": ["https://associatedsite1.test"]})"));

  EXPECT_THAT(
      GetSetsAndWait().FindEntries({example, associated},
                                   net::FirstPartySetsContextConfig()),
      UnorderedElementsAre(
          Pair(example, net::FirstPartySetEntry(
                            example, net::SiteType::kPrimary, absl::nullopt)),
          Pair(associated, net::FirstPartySetEntry(
                               example, net::SiteType::kAssociated, 0))));
}

TEST_F(FirstPartySetsHandlerImplEnabledTest,
       ClearSiteDataOnChangedSetsForContext_FeatureNotEnabled) {
  base::HistogramTester histogram;
  net::SchemefulSite foo(GURL("https://foo.test"));
  net::SchemefulSite associated(GURL("https://associatedsite.test"));

  FirstPartySetsHandlerImpl::GetInstance()
      ->SetEmbedderWillProvidePublicSetsForTesting(true);
  const std::string browser_context_id = "profile";
  const std::string input =
      R"({"primary": "https://foo.test", )"
      R"("associatedSites": ["https://associatedsite.test"]})";
  ASSERT_TRUE(base::JSONReader::Read(input));
  FirstPartySetsHandlerImpl::GetInstance()->SetPublicFirstPartySets(
      base::Version("0.0.1"), WritePublicSetsFile(input));

  FirstPartySetsHandlerImpl::GetInstance()->Init(scoped_dir_.GetPath(),
                                                 LocalSetDeclaration());
  ASSERT_THAT(GetSetsAndWait().FindEntries({foo, associated},
                                           net::FirstPartySetsContextConfig()),
              UnorderedElementsAre(
                  Pair(foo, net::FirstPartySetEntry(
                                foo, net::SiteType::kPrimary, absl::nullopt)),
                  Pair(associated, net::FirstPartySetEntry(
                                       foo, net::SiteType::kAssociated, 0))));

  base::RunLoop run_loop;
  FirstPartySetsHandlerImpl::GetInstance()
      ->ClearSiteDataOnChangedSetsForContext(
          base::BindLambdaForTesting([&]() { return context(); }),
          browser_context_id, net::FirstPartySetsContextConfig(),
          base::BindLambdaForTesting(
              [&](net::FirstPartySetsContextConfig,
                  net::FirstPartySetsCacheFilter) { run_loop.Quit(); }));
  run_loop.Run();

  EXPECT_THAT(
      GetPersistedGlobalSetsAndWait(browser_context_id)
          ->FindEntries({foo, associated}, net::FirstPartySetsContextConfig()),
      IsEmpty());
  // Should not be recorded.
  histogram.ExpectTotalCount(kFirstPartySetsClearSiteDataOutcomeHistogram, 0);
}

TEST_F(FirstPartySetsHandlerImplEnabledTest,
       ClearSiteDataOnChangedSetsForContext_Successful) {
  base::test::ScopedFeatureList features;
  features.InitAndEnableFeatureWithParameters(
      features::kFirstPartySets,
      {{features::kFirstPartySetsClearSiteDataOnChangedSets.name, "true"}});

  base::HistogramTester histogram;
  net::SchemefulSite foo(GURL("https://foo.test"));
  net::SchemefulSite associated(GURL("https://associatedsite.test"));

  FirstPartySetsHandlerImpl::GetInstance()
      ->SetEmbedderWillProvidePublicSetsForTesting(true);
  const std::string browser_context_id = "profile";
  const std::string input =
      R"({"primary": "https://foo.test", )"
      R"("associatedSites": ["https://associatedsite.test"]})";
  ASSERT_TRUE(base::JSONReader::Read(input));
  FirstPartySetsHandlerImpl::GetInstance()->SetPublicFirstPartySets(
      base::Version("0.0.1"), WritePublicSetsFile(input));

  FirstPartySetsHandlerImpl::GetInstance()->Init(scoped_dir_.GetPath(),
                                                 LocalSetDeclaration());

  EXPECT_THAT(HasEntryInBrowserContextsClearedAndWait(browser_context_id),
              Optional(false));

  ASSERT_THAT(GetSetsAndWait().FindEntries({foo, associated},
                                           net::FirstPartySetsContextConfig()),
              UnorderedElementsAre(
                  Pair(foo, net::FirstPartySetEntry(
                                foo, net::SiteType::kPrimary, absl::nullopt)),
                  Pair(associated, net::FirstPartySetEntry(
                                       foo, net::SiteType::kAssociated, 0))));

  // Should not yet be recorded.
  histogram.ExpectTotalCount(kFirstPartySetsClearSiteDataOutcomeHistogram, 0);
  base::RunLoop run_loop;
  FirstPartySetsHandlerImpl::GetInstance()
      ->ClearSiteDataOnChangedSetsForContext(
          base::BindLambdaForTesting([&]() { return context(); }),
          browser_context_id, net::FirstPartySetsContextConfig(),
          base::BindLambdaForTesting(
              [&](net::FirstPartySetsContextConfig,
                  net::FirstPartySetsCacheFilter) { run_loop.Quit(); }));
  run_loop.Run();

  EXPECT_THAT(
      GetPersistedGlobalSetsAndWait(browser_context_id)
          ->FindEntries({foo, associated}, net::FirstPartySetsContextConfig()),
      UnorderedElementsAre(
          Pair(foo, net::FirstPartySetEntry(foo, net::SiteType::kPrimary,
                                            absl::nullopt)),
          Pair(associated,
               net::FirstPartySetEntry(foo, net::SiteType::kAssociated,
                                       absl::nullopt))));
  EXPECT_THAT(HasEntryInBrowserContextsClearedAndWait(browser_context_id),
              Optional(true));

  histogram.ExpectUniqueSample(
      kFirstPartySetsClearSiteDataOutcomeHistogram,
      FirstPartySetsHandlerImpl::ClearSiteDataOutcomeType::kSuccess, 1);
}

TEST_F(FirstPartySetsHandlerImplEnabledTest,
       ClearSiteDataOnChangedSetsForContext_EmptyDBPath) {
  base::test::ScopedFeatureList features;
  features.InitAndEnableFeatureWithParameters(
      features::kFirstPartySets,
      {{features::kFirstPartySetsClearSiteDataOnChangedSets.name, "true"}});

  base::HistogramTester histogram;
  net::SchemefulSite foo(GURL("https://foo.test"));
  net::SchemefulSite associated(GURL("https://associatedsite.test"));

  FirstPartySetsHandlerImpl::GetInstance()
      ->SetEmbedderWillProvidePublicSetsForTesting(true);
  const std::string browser_context_id = "profile";
  const std::string input =
      R"({"primary": "https://foo.test", )"
      R"("associatedSites": ["https://associatedsite.test"]})";
  ASSERT_TRUE(base::JSONReader::Read(input));
  FirstPartySetsHandlerImpl::GetInstance()->SetPublicFirstPartySets(
      base::Version("0.0.1"), WritePublicSetsFile(input));

  FirstPartySetsHandlerImpl::GetInstance()->Init(
      /*user_data_dir=*/{}, LocalSetDeclaration());
  ASSERT_THAT(GetSetsAndWait().FindEntries({foo, associated},
                                           net::FirstPartySetsContextConfig()),
              UnorderedElementsAre(
                  Pair(foo, net::FirstPartySetEntry(
                                foo, net::SiteType::kPrimary, absl::nullopt)),
                  Pair(associated, net::FirstPartySetEntry(
                                       foo, net::SiteType::kAssociated, 0))));

  base::RunLoop run_loop;
  FirstPartySetsHandlerImpl::GetInstance()
      ->ClearSiteDataOnChangedSetsForContext(
          base::BindLambdaForTesting([&]() { return context(); }),
          browser_context_id, net::FirstPartySetsContextConfig(),
          base::BindLambdaForTesting(
              [&](net::FirstPartySetsContextConfig,
                  net::FirstPartySetsCacheFilter) { run_loop.Quit(); }));
  run_loop.Run();

  EXPECT_EQ(GetPersistedGlobalSetsAndWait(browser_context_id), absl::nullopt);
  // Should not be recorded.
  histogram.ExpectTotalCount(kFirstPartySetsClearSiteDataOutcomeHistogram, 0);
}

TEST_F(FirstPartySetsHandlerImplEnabledTest,
       ClearSiteDataOnChangedSetsForContext_BeforeSetsReady) {
  base::test::ScopedFeatureList features;
  features.InitAndEnableFeatureWithParameters(
      features::kFirstPartySets,
      {{features::kFirstPartySetsClearSiteDataOnChangedSets.name, "true"}});

  base::HistogramTester histogram;
  FirstPartySetsHandlerImpl::GetInstance()
      ->SetEmbedderWillProvidePublicSetsForTesting(true);

  FirstPartySetsHandlerImpl::GetInstance()->Init(scoped_dir_.GetPath(),
                                                 LocalSetDeclaration());

  const std::string browser_context_id = "profile";
  base::test::TestFuture<net::FirstPartySetsContextConfig,
                         net::FirstPartySetsCacheFilter>
      future;
  FirstPartySetsHandlerImpl::GetInstance()
      ->ClearSiteDataOnChangedSetsForContext(
          base::BindLambdaForTesting([&]() { return context(); }),
          browser_context_id, net::FirstPartySetsContextConfig(),
          future.GetCallback());

  FirstPartySetsHandlerImpl::GetInstance()->SetPublicFirstPartySets(
      base::Version("0.0.1"),
      WritePublicSetsFile(
          R"({"primary": "https://foo.test", )"
          R"("associatedSites": ["https://associatedsite.test"]})"));

  EXPECT_TRUE(future.Wait());

  net::SchemefulSite foo(GURL("https://foo.test"));
  net::SchemefulSite associated(GURL("https://associatedsite.test"));
  EXPECT_THAT(
      GetPersistedGlobalSetsAndWait(browser_context_id)
          ->FindEntries({foo, associated}, net::FirstPartySetsContextConfig()),
      UnorderedElementsAre(
          Pair(foo, net::FirstPartySetEntry(foo, net::SiteType::kPrimary,
                                            absl::nullopt)),
          Pair(associated,
               net::FirstPartySetEntry(foo, net::SiteType::kAssociated,
                                       absl::nullopt))));
  histogram.ExpectUniqueSample(
      kFirstPartySetsClearSiteDataOutcomeHistogram,
      FirstPartySetsHandlerImpl::ClearSiteDataOutcomeType::kSuccess, 1);
}

TEST_F(FirstPartySetsHandlerImplEnabledTest,
       GetSetsIfEnabledAndReady_AfterSetsReady) {
  net::SchemefulSite example(GURL("https://example.test"));
  net::SchemefulSite associated(GURL("https://associatedsite.test"));

  FirstPartySetsHandlerImpl::GetInstance()
      ->SetEmbedderWillProvidePublicSetsForTesting(true);

  const std::string input =
      R"({"primary": "https://example.test", )"
      R"("associatedSites": ["https://associatedsite.test"]})";
  ASSERT_TRUE(base::JSONReader::Read(input));
  FirstPartySetsHandlerImpl::GetInstance()->SetPublicFirstPartySets(
      base::Version(), WritePublicSetsFile(input));

  FirstPartySetsHandlerImpl::GetInstance()->Init(scoped_dir_.GetPath(),
                                                 LocalSetDeclaration());

  // Wait until initialization is complete.
  GetSetsAndWait();

  EXPECT_THAT(
      FirstPartySetsHandlerImpl::GetInstance()
          ->GetSets(base::NullCallback())
          .value()
          .FindEntries({example, associated},
                       net::FirstPartySetsContextConfig()),
      UnorderedElementsAre(
          Pair(example, net::FirstPartySetEntry(
                            example, net::SiteType::kPrimary, absl::nullopt)),
          Pair(associated, net::FirstPartySetEntry(
                               example, net::SiteType::kAssociated, 0))));
}

TEST_F(FirstPartySetsHandlerImplEnabledTest,
       GetSetsIfEnabledAndReady_BeforeSetsReady) {
  net::SchemefulSite example(GURL("https://example.test"));
  net::SchemefulSite associated(GURL("https://associatedsite.test"));

  FirstPartySetsHandlerImpl::GetInstance()
      ->SetEmbedderWillProvidePublicSetsForTesting(true);

  // Call GetSets before the sets are ready, and before Init has been called.
  base::test::TestFuture<net::GlobalFirstPartySets> future;
  EXPECT_EQ(
      FirstPartySetsHandlerImpl::GetInstance()->GetSets(future.GetCallback()),
      absl::nullopt);

  FirstPartySetsHandlerImpl::GetInstance()->Init(scoped_dir_.GetPath(),
                                                 LocalSetDeclaration());

  const std::string input =
      R"({"primary": "https://example.test", )"
      R"("associatedSites": ["https://associatedsite.test"]})";
  ASSERT_TRUE(base::JSONReader::Read(input));
  FirstPartySetsHandlerImpl::GetInstance()->SetPublicFirstPartySets(
      base::Version(), WritePublicSetsFile(input));

  EXPECT_THAT(
      future.Get().FindEntries({example, associated},
                               net::FirstPartySetsContextConfig()),
      UnorderedElementsAre(
          Pair(example, net::FirstPartySetEntry(
                            example, net::SiteType::kPrimary, absl::nullopt)),
          Pair(associated, net::FirstPartySetEntry(
                               example, net::SiteType::kAssociated, 0))));

  EXPECT_THAT(
      FirstPartySetsHandlerImpl::GetInstance()
          ->GetSets(base::NullCallback())
          .value()
          .FindEntries({example, associated},
                       net::FirstPartySetsContextConfig()),
      UnorderedElementsAre(
          Pair(example, net::FirstPartySetEntry(
                            example, net::SiteType::kPrimary, absl::nullopt)),
          Pair(associated, net::FirstPartySetEntry(
                               example, net::SiteType::kAssociated, 0))));
}

class FirstPartySetsHandlerGetContextConfigForPolicyTest
    : public FirstPartySetsHandlerImplEnabledTest {
 public:
  FirstPartySetsHandlerGetContextConfigForPolicyTest() {
    FirstPartySetsHandlerImpl::GetInstance()
        ->SetEmbedderWillProvidePublicSetsForTesting(true);
    FirstPartySetsHandlerImpl::GetInstance()->Init(scoped_dir_.GetPath(),
                                                   LocalSetDeclaration());
  }

  // Writes the public list of First-Party Sets which GetContextConfigForPolicy
  // awaits.
  //
  // Initializes the First-Party Sets with the following relationship:
  //
  // [
  //   {
  //     "primary": "https://primary1.test",
  //     "associatedSites": ["https://associatedsite1.test",
  //     "https://associatedsite2.test"]
  //   }
  // ]
  void InitPublicFirstPartySets() {
    net::SchemefulSite primary1(GURL("https://primary1.test"));
    net::SchemefulSite associated1(GURL("https://associatedsite1.test"));
    net::SchemefulSite associated2(GURL("https://associatedsite2.test"));

    const std::string input =
        R"({"primary": "https://primary1.test", )"
        R"("associatedSites": ["https://associatedsite1.test", "https://associatedsite2.test"]})";
    ASSERT_TRUE(base::JSONReader::Read(input));
    FirstPartySetsHandlerImpl::GetInstance()->SetPublicFirstPartySets(
        base::Version(), WritePublicSetsFile(input));

    ASSERT_THAT(
        GetSetsAndWait().FindEntries({primary1, associated1, associated2},
                                     net::FirstPartySetsContextConfig()),
        SizeIs(3));
  }

 protected:
  base::OnceCallback<void(net::FirstPartySetsContextConfig)>
  GetConfigCallback() {
    return future_.GetCallback();
  }

  net::FirstPartySetsContextConfig GetConfig() { return future_.Take(); }

 private:
  base::test::TestFuture<net::FirstPartySetsContextConfig> future_;
};

TEST_F(FirstPartySetsHandlerGetContextConfigForPolicyTest,
       DefaultOverridesPolicy_DefaultContextConfigs) {
  base::Value policy = base::JSONReader::Read(R"({})").value();
  FirstPartySetsHandlerImpl::GetInstance()->GetContextConfigForPolicy(
      &policy.GetDict(), GetConfigCallback());

  InitPublicFirstPartySets();
  EXPECT_EQ(GetConfig(), net::FirstPartySetsContextConfig());
}

TEST_F(FirstPartySetsHandlerGetContextConfigForPolicyTest,
       MalformedOverridesPolicy_DefaultContextConfigs) {
  base::Value policy = base::JSONReader::Read(R"({
    "replacements": 123,
    "additions": true
  })")
                           .value();
  FirstPartySetsHandlerImpl::GetInstance()->GetContextConfigForPolicy(
      &policy.GetDict(), GetConfigCallback());

  InitPublicFirstPartySets();
  EXPECT_EQ(GetConfig(), net::FirstPartySetsContextConfig());
}

TEST_F(FirstPartySetsHandlerGetContextConfigForPolicyTest,
       NonDefaultOverridesPolicy_NonDefaultContextConfigs) {
  base::Value policy = base::JSONReader::Read(R"(
                {
                "replacements": [
                  {
                    "primary": "https://associatedsite1.test",
                    "associatedSites": ["https://primary3.test"]
                  }
                ],
                "additions": [
                  {
                    "primary": "https://primary2.test",
                    "associatedSites": ["https://associatedsite2.test"]
                  }
                ]
              }
            )")
                           .value();
  FirstPartySetsHandlerImpl::GetInstance()->GetContextConfigForPolicy(
      &policy.GetDict(), GetConfigCallback());

  InitPublicFirstPartySets();
  // We don't care what the customizations are, here; we only care that they're
  // not a no-op.
  EXPECT_FALSE(GetConfig().empty());
}

}  // namespace content
