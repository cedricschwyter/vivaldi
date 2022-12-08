// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/parser/html_document_parser.h"

#include <memory>

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/blink/renderer/core/html/html_document.h"
#include "third_party/blink/renderer/core/html/parser/html_token_producer.h"
#include "third_party/blink/renderer/core/html/parser/text_resource_decoder.h"
#include "third_party/blink/renderer/core/html/parser/text_resource_decoder_builder.h"
#include "third_party/blink/renderer/core/loader/no_state_prefetch_client.h"
#include "third_party/blink/renderer/core/testing/page_test_base.h"
#include "third_party/blink/renderer/core/testing/sim/sim_request.h"
#include "third_party/blink/renderer/core/testing/sim/sim_test.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/testing/runtime_enabled_features_test_helpers.h"
#include "third_party/blink/renderer/platform/testing/unit_test_helpers.h"

namespace blink {

namespace {

class MockNoStatePrefetchClient : public NoStatePrefetchClient {
 public:
  MockNoStatePrefetchClient(Page& page, bool is_prefetch_only)
      : NoStatePrefetchClient(page, nullptr),
        is_prefetch_only_(is_prefetch_only) {}

 private:
  bool IsPrefetchOnly() override { return is_prefetch_only_; }

  bool is_prefetch_only_;
};

class HTMLDocumentParserTest
    : public PageTestBase,
      public testing::WithParamInterface<ParserSynchronizationPolicy> {
 protected:
  HTMLDocumentParserTest()
      : original_force_synchronous_parsing_for_testing_(
            Document::ForceSynchronousParsingForTesting()) {
    Document::SetForceSynchronousParsingForTesting(Policy() ==
                                                   kForceSynchronousParsing);
  }
  ~HTMLDocumentParserTest() override {
    // Finish the pending tasks which may require the runtime enabled flags,
    // before restoring the flags.
    base::RunLoop().RunUntilIdle();
    Document::SetForceSynchronousParsingForTesting(
        original_force_synchronous_parsing_for_testing_);
  }

  void SetUp() override {
    PageTestBase::SetUp();
    GetDocument().SetURL(KURL("https://example.test"));
  }

  HTMLDocumentParser* CreateParser(HTMLDocument& document) {
    auto* parser =
        MakeGarbageCollected<HTMLDocumentParser>(document, GetParam());
    std::unique_ptr<TextResourceDecoder> decoder(BuildTextResourceDecoder(
        document.GetFrame(), document.Url(), "text/html", g_null_atom));
    parser->SetDecoder(std::move(decoder));
    return parser;
  }

 private:
  ParserSynchronizationPolicy Policy() const { return GetParam(); }

  bool original_force_synchronous_parsing_for_testing_;
};

// Calls DocumentParser::Detach() in the destructor. Used to ensure detach is
// called, as otherwise some assertions may be triggered.
class ScopedParserDetacher {
 public:
  explicit ScopedParserDetacher(DocumentParser* parser) : parser_(parser) {}

  explicit ScopedParserDetacher(HTMLDocumentParser* parser)
      : ScopedParserDetacher(static_cast<DocumentParser*>(parser)) {}

  ~ScopedParserDetacher() { parser_->Detach(); }

 private:
  UntracedMember<DocumentParser> parser_;
};

}  // namespace

INSTANTIATE_TEST_SUITE_P(HTMLDocumentParserTest,
                         HTMLDocumentParserTest,
                         testing::Values(kForceSynchronousParsing,
                                         kAllowDeferredParsing));

TEST_P(HTMLDocumentParserTest, StopThenPrepareToStopShouldNotCrash) {
  auto& document = To<HTMLDocument>(GetDocument());
  DocumentParser* parser = CreateParser(document);
  ScopedParserDetacher detacher(parser);
  const char kBytes[] = "<html>";
  parser->AppendBytes(kBytes, sizeof(kBytes));
  // These methods are not supposed to be called one after the other, but in
  // practice it can happen (e.g. if navigation is aborted).
  parser->StopParsing();
  parser->PrepareToStopParsing();
}

TEST_P(HTMLDocumentParserTest, HasNoPendingWorkAfterStopParsing) {
  auto& document = To<HTMLDocument>(GetDocument());
  HTMLDocumentParser* parser = CreateParser(document);
  DocumentParser* control_parser = static_cast<DocumentParser*>(parser);
  ScopedParserDetacher detacher(control_parser);
  const char kBytes[] = "<html>";
  control_parser->AppendBytes(kBytes, sizeof(kBytes));
  control_parser->StopParsing();
  EXPECT_FALSE(parser->HasPendingWorkScheduledForTesting());
}

TEST_P(HTMLDocumentParserTest, HasNoPendingWorkAfterStopParsingThenAppend) {
  auto& document = To<HTMLDocument>(GetDocument());
  HTMLDocumentParser* parser = CreateParser(document);
  DocumentParser* control_parser = static_cast<DocumentParser*>(parser);
  ScopedParserDetacher detacher(control_parser);
  const char kBytes1[] = "<html>";
  control_parser->AppendBytes(kBytes1, sizeof(kBytes1));
  control_parser->StopParsing();
  const char kBytes2[] = "<head>";
  control_parser->AppendBytes(kBytes2, sizeof(kBytes2));
  EXPECT_FALSE(parser->HasPendingWorkScheduledForTesting());
}

TEST_P(HTMLDocumentParserTest, HasNoPendingWorkAfterDetach) {
  auto& document = To<HTMLDocument>(GetDocument());
  HTMLDocumentParser* parser = CreateParser(document);
  DocumentParser* control_parser = static_cast<DocumentParser*>(parser);
  const char kBytes[] = "<html>";
  control_parser->AppendBytes(kBytes, sizeof(kBytes));
  control_parser->Detach();
  EXPECT_FALSE(parser->HasPendingWorkScheduledForTesting());
}

TEST_P(HTMLDocumentParserTest, AppendPrefetch) {
  auto& document = To<HTMLDocument>(GetDocument());
  ProvideNoStatePrefetchClientTo(
      *document.GetPage(), MakeGarbageCollected<MockNoStatePrefetchClient>(
                               *document.GetPage(), true));
  EXPECT_TRUE(document.IsPrefetchOnly());
  HTMLDocumentParser* parser = CreateParser(document);
  ScopedParserDetacher detacher(parser);

  const char kBytes[] = "<httttttt";
  parser->AppendBytes(kBytes, sizeof(kBytes));
  // The bytes are forwarded to the preload scanner, not to the tokenizer.
  HTMLParserScriptRunnerHost* script_runner_host =
      parser->AsHTMLParserScriptRunnerHostForTesting();
  EXPECT_TRUE(script_runner_host->HasPreloadScanner());
  // Finishing should not cause parsing to start (verified via an internal
  // DCHECK).
  EXPECT_FALSE(parser->DidPumpTokenizerForTesting());
  static_cast<DocumentParser*>(parser)->Finish();
  EXPECT_FALSE(parser->DidPumpTokenizerForTesting());
  // Cancel any pending work to make sure that RuntimeFeatures DCHECKs do not
  // fire.
  static_cast<DocumentParser*>(parser)->StopParsing();
}

TEST_P(HTMLDocumentParserTest, AppendNoPrefetch) {
  auto& document = To<HTMLDocument>(GetDocument());
  EXPECT_FALSE(document.IsPrefetchOnly());
  // Use ForceSynchronousParsing to allow calling append().
  HTMLDocumentParser* parser = CreateParser(document);
  ScopedParserDetacher detacher(parser);

  const char kBytes[] = "<htttttt";
  parser->AppendBytes(kBytes, sizeof(kBytes));
  test::RunPendingTasks();
  // The bytes are forwarded to the tokenizer.
  HTMLParserScriptRunnerHost* script_runner_host =
      parser->AsHTMLParserScriptRunnerHostForTesting();
  EXPECT_EQ(script_runner_host->HasPreloadScanner(),
            GetParam() == kAllowDeferredParsing);
  EXPECT_TRUE(parser->DidPumpTokenizerForTesting());
  // Cancel any pending work to make sure that RuntimeFeatures DCHECKs do not
  // fire.
  static_cast<DocumentParser*>(parser)->StopParsing();
}

class HTMLDocumentParserThreadedPreloadScannerTest : public PageTestBase {
 protected:
  HTMLDocumentParserThreadedPreloadScannerTest() {
    scoped_feature_list_.InitWithFeatures(
        {features::kThreadedPreloadScanner, features::kPrecompileInlineScripts},
        {});
    HTMLDocumentParser::ResetCachedFeaturesForTesting();
  }

  ~HTMLDocumentParserThreadedPreloadScannerTest() override {
    scoped_feature_list_.Reset();
    HTMLDocumentParser::ResetCachedFeaturesForTesting();
  }

  void SetUp() override {
    PageTestBase::SetUp();
    GetDocument().SetURL(KURL("https://example.test"));
  }

  HTMLDocumentParser* CreateParser(HTMLDocument& document) {
    return MakeGarbageCollected<HTMLDocumentParser>(document,
                                                    kAllowDeferredParsing);
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_F(HTMLDocumentParserThreadedPreloadScannerTest,
       TakeBackgroundScanCallback) {
  auto& document = To<HTMLDocument>(GetDocument());
  HTMLDocumentParser* parser = CreateParser(document);
  ScopedParserDetacher detacher(parser);

  // First append "foo" script which should be passed through to the scanner.
  parser->AppendDecodedData("<script>foo</script>", DocumentEncodingData());
  HTMLDocumentParser::FlushPreloadScannerThreadForTesting();
  EXPECT_TRUE(parser->HasInlineScriptStreamerForTesting("foo"));

  // Now take the callback.
  auto callback =
      static_cast<DocumentParser*>(parser)->TakeBackgroundScanCallback();

  // Append "bar" script which should not be passed to the scanner.
  parser->AppendDecodedData("<script>bar</script>", DocumentEncodingData());
  HTMLDocumentParser::FlushPreloadScannerThreadForTesting();
  EXPECT_FALSE(parser->HasInlineScriptStreamerForTesting("bar"));

  // Append "baz" script to the callback which should be passed to the scanner.
  callback.Run("<script>baz</script>");
  HTMLDocumentParser::FlushPreloadScannerThreadForTesting();
  EXPECT_TRUE(parser->HasInlineScriptStreamerForTesting("baz"));

  static_cast<DocumentParser*>(parser)->StopParsing();
}

class HTMLDocumentParserProcessImmediatelyTest : public PageTestBase {
 protected:
  void SetUp() override {
    PageTestBase::SetUp();
    GetDocument().SetURL(KURL("https://example.test"));
  }

  HTMLDocumentParser* CreateParser(HTMLDocument& document) {
    auto* parser = MakeGarbageCollected<HTMLDocumentParser>(
        document, kAllowDeferredParsing);
    std::unique_ptr<TextResourceDecoder> decoder(BuildTextResourceDecoder(
        document.GetFrame(), document.Url(), "text/html", g_null_atom));
    parser->SetDecoder(std::move(decoder));
    return parser;
  }

 private:
};

TEST_F(HTMLDocumentParserProcessImmediatelyTest, FirstChunk) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      features::kProcessHtmlDataImmediately,
      {{features::kProcessHtmlDataImmediatelyFirstChunk.name, "true"}});
  auto& document = To<HTMLDocument>(GetDocument());
  HTMLDocumentParser* parser = CreateParser(document);
  ScopedParserDetacher detacher(parser);
  const char kBytes[] = "<htttttt";
  parser->AppendBytes(kBytes, sizeof(kBytes));
  // Because kProcessHtmlDataImmediatelyFirstChunk is set,
  // DidPumpTokenizerForTesting() should be true.
  EXPECT_TRUE(parser->DidPumpTokenizerForTesting());
  // Cancel any pending work to make sure that RuntimeFeatures DCHECKs do not
  // fire.
  static_cast<DocumentParser*>(parser)->StopParsing();
}

TEST_F(HTMLDocumentParserProcessImmediatelyTest, SecondChunk) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      features::kProcessHtmlDataImmediately,
      {{features::kProcessHtmlDataImmediatelySubsequentChunks.name, "true"}});
  auto& document = To<HTMLDocument>(GetDocument());
  HTMLDocumentParser* parser = CreateParser(document);
  ScopedParserDetacher detacher(parser);
  const char kBytes[] = "<div><div><div>";
  parser->AppendBytes(kBytes, sizeof(kBytes) - 1);
  // The first chunk should not have been processed yet (it was scheduled).
  EXPECT_FALSE(parser->DidPumpTokenizerForTesting());
  test::RunPendingTasks();
  EXPECT_TRUE(parser->DidPumpTokenizerForTesting());
  EXPECT_EQ(1u, parser->GetChunkCountForTesting());
  parser->AppendBytes(kBytes, sizeof(kBytes) - 1);
  // As kProcessHtmlDataImmediatelySubsequentChunks is true, the second chunk
  // should be processed immediately.
  EXPECT_EQ(2u, parser->GetChunkCountForTesting());
  // Cancel any pending work to make sure that RuntimeFeatures DCHECKs do not
  // fire.
  static_cast<DocumentParser*>(parser)->StopParsing();
}

class HTMLDocumentParserWithThreadedTokenizerTest : public SimTest {
 protected:
  HTMLDocumentParserWithThreadedTokenizerTest()
      : original_force_synchronous_parsing_for_testing_(
            Document::ForceSynchronousParsingForTesting()) {
    Document::SetForceSynchronousParsingForTesting(false);
  }
  ~HTMLDocumentParserWithThreadedTokenizerTest() override {
    // Finish the pending tasks which may require the runtime enabled flags,
    // before restoring the flags.
    base::RunLoop().RunUntilIdle();
    Document::SetForceSynchronousParsingForTesting(
        original_force_synchronous_parsing_for_testing_);
  }

  void SetUp() override {
    SimTest::SetUp();
    scoped_feature_list_.InitAndEnableFeature(features::kThreadedHtmlTokenizer);
    GetDocument().SetURL(KURL("https://example.test"));
  }

 private:
  const bool original_force_synchronous_parsing_for_testing_;
  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_F(HTMLDocumentParserWithThreadedTokenizerTest,
       LoadedUrlUsesBackgroundTokenizer) {
  SimRequest main_resource("https://example.com/test.html", "text/html");
  LoadURL("https://example.com/test.html");
  ASSERT_TRUE(GetDocument().Parser());
  auto* token_producer =
      static_cast<HTMLDocumentParser*>(GetDocument().Parser())
          ->TokenProducerForTesting();
  // For normal loading the background tokenizer should be used (with the
  // feature enabled).
  EXPECT_TRUE(token_producer->IsUsingBackgroundProducer());
  main_resource.Complete("<head>");
  test::RunPendingTasks();
}

TEST_F(HTMLDocumentParserWithThreadedTokenizerTest,
       EmptyDocumentDoesNotUseBackgroundTokenizer) {
  DocumentInit init = DocumentInit::Create().ForInitialEmptyDocument(true);
  HTMLDocument* empty_doc = MakeGarbageCollected<HTMLDocument>(init);
  ASSERT_TRUE(empty_doc->IsInitialEmptyDocument());
  HTMLDocumentParser* parser = MakeGarbageCollected<HTMLDocumentParser>(
      *empty_doc, kAllowDeferredParsing);
  // Empty documents should not use the background tokenizer.
  EXPECT_FALSE(parser->TokenProducerForTesting()->IsUsingBackgroundProducer());
}

}  // namespace blink
