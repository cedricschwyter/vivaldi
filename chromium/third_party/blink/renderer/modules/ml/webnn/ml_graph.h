// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_ML_WEBNN_ML_GRAPH_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_ML_WEBNN_ML_GRAPH_H_

#include "third_party/blink/renderer/bindings/modules/v8/v8_ml_operand_descriptor.h"
#include "third_party/blink/renderer/modules/ml/webnn/ml_graph_builder.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/member.h"
#include "third_party/blink/renderer/platform/heap/visitor.h"
#include "third_party/blink/renderer/platform/wtf/text/string_hash.h"

namespace blink {

class MLContext;
class ScriptPromiseResolver;

class MODULES_EXPORT MLGraph : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  MLGraph(const MLGraph&) = delete;
  MLGraph& operator=(const MLGraph&) = delete;

  ~MLGraph() override;

  void Trace(Visitor* visitor) const override;

  // The members of ResourceInfo are used to validate the inputs and outputs of
  // an MLGraph execution. The validation steps are described by WebNN spec of
  // MLContext.computeAsync() and MLContext.compute() methods:
  // https://www.w3.org/TR/webnn/#api-mlcontext-async-execution
  // https://www.w3.org/TR/webnn/#api-mlcontext-sync-execution
  // The plain struct ResourceInfo is introduced instead of using
  // MLOperandDescriptor because neither byte length calculation from dimensions
  // nor GC support is needed for the implementation.
  struct ResourceInfo {
    V8MLOperandType::Enum type;
    size_t byte_length;
  };
  const HashMap<String, ResourceInfo>& GetInputResourcesInfo() const;
  const HashMap<String, ResourceInfo>& GetOutputResourcesInfo() const;

 protected:
  explicit MLGraph(MLContext* context);

  // BuildAsync() should be called right after constructing a concrete
  // MLGraph object. FakeMLGraphBackend::ValidateAndBuildAsync() in
  // ml_graph_builder_test.cc gives an example. BuildAsync() validates the named
  // outputs and initializes the input and output resources info. If there are
  // no errors, it calls BuildAsyncImpl() implemented by an MLGraph backend that
  // builds the platform specific graph.
  void BuildAsync(const MLNamedOperands& named_outputs,
                  ScriptPromiseResolver* resolver);

  // An MLGraph backend should implement this method to build and compile a
  // platform specific graph asynchronously. The actual graph construction and
  // compilation work should be handled by a worker thread without blocking the
  // main thread. Once the platform graph is compiled, the resolver should be
  // resolved with a concrete MLGraph object. Otherwise, the resolver should be
  // rejected with a DOMException accordingly.
  virtual void BuildAsyncImpl(const MLNamedOperands& outputs,
                              ScriptPromiseResolver* resolver) = 0;

  Member<MLContext> ml_context_;
  bool resources_info_initialized_{false};
  HashMap<String, ResourceInfo> input_resources_info_;
  HashMap<String, ResourceInfo> output_resources_info_;

 private:
  // This helper method is called by BuildAsync(). It validates named outputs
  // and initializes the input and output resources info by graph traversal.
  bool ValidateAndInitializeResourcesInfo(const MLNamedOperands& named_outputs,
                                          String& error_message);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_ML_WEBNN_ML_GRAPH_H_
