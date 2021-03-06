/*
 * Copyright 2014 Google Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef KYTHE_CXX_COMMON_INDEXING_KYTHE_OUTPUT_STREAM_H_
#define KYTHE_CXX_COMMON_INDEXING_KYTHE_OUTPUT_STREAM_H_

#include "absl/strings/string_view.h"

#include "kythe/proto/common.pb.h"
#include "kythe/proto/storage.pb.h"

namespace kythe {
/// \brief Code marked with semantic spans.
using MarkedSource = kythe::proto::common::MarkedSource;

/// A collection of references to the components of a VName.
struct VNameRef {
  absl::string_view signature;
  absl::string_view corpus;
  absl::string_view root;
  absl::string_view path;
  absl::string_view language;
  explicit VNameRef(const proto::VName &vname)
      : signature(vname.signature().data(), vname.signature().size()),
        corpus(vname.corpus().data(), vname.corpus().size()),
        root(vname.root().data(), vname.root().size()),
        path(vname.path().data(), vname.path().size()),
        language(vname.language().data(), vname.language().size()) {}
  VNameRef() {}
  void Expand(proto::VName *vname) const {
    vname->mutable_signature()->assign(signature.data(), signature.size());
    vname->mutable_corpus()->assign(corpus.data(), corpus.size());
    vname->mutable_root()->assign(root.data(), root.size());
    vname->mutable_path()->assign(path.data(), path.size());
    vname->mutable_language()->assign(language.data(), language.size());
  }
};
/// A collection of references to the components of a single Kythe fact.
struct FactRef {
  const VNameRef *source;
  absl::string_view fact_name;
  absl::string_view fact_value;
  /// Overwrites all of the fields in `entry` that can differ between single
  /// facts.
  void Expand(proto::Entry *entry) const {
    source->Expand(entry->mutable_source());
    entry->mutable_fact_name()->assign(fact_name.data(), fact_name.size());
    entry->mutable_fact_value()->assign(fact_value.data(), fact_value.size());
  }
};
/// A collection of references to the components of a single Kythe edge.
struct EdgeRef {
  const VNameRef *source;
  absl::string_view edge_kind;
  const VNameRef *target;
  /// Overwrites all of the fields in `entry` that can differ between edges
  /// without ordinals.
  void Expand(proto::Entry *entry) const {
    source->Expand(entry->mutable_source());
    target->Expand(entry->mutable_target());
    entry->mutable_edge_kind()->assign(edge_kind.data(), edge_kind.size());
  }
};
/// A collection of references to the components of a single Kythe edge with an
/// ordinal.
struct OrdinalEdgeRef {
  const VNameRef *source;
  absl::string_view edge_kind;
  const VNameRef *target;
  uint32_t ordinal;
  /// Overwrites all of the fields in `entry` that can differ between edges with
  /// ordinals.
  void Expand(proto::Entry *entry) const {
    char digits[12];  // strlen("4294967295") + 2
    int dot_ordinal_length = ::sprintf(digits, ".%u", ordinal);
    entry->mutable_edge_kind()->clear();
    entry->mutable_edge_kind()->reserve(dot_ordinal_length + edge_kind.size());
    entry->mutable_edge_kind()->append(edge_kind.data(), edge_kind.size());
    entry->mutable_edge_kind()->append(digits, dot_ordinal_length);
    source->Expand(entry->mutable_source());
    target->Expand(entry->mutable_target());
  }
};

// Interface for receiving Kythe data.
class KytheOutputStream {
 public:
  virtual void Emit(const FactRef &fact) = 0;
  virtual void Emit(const EdgeRef &edge) = 0;
  virtual void Emit(const OrdinalEdgeRef &edge) = 0;
  /// Add a buffer to the buffer stack to group facts, edges, and buffers
  /// together.
  virtual void PushBuffer() {}
  /// Pop the last buffer from the buffer stack.
  virtual void PopBuffer() {}
  virtual ~KytheOutputStream() {}
};

}  // namespace kythe

#endif  // KYTHE_CXX_COMMON_INDEXING_KYTHE_OUTPUT_STREAM_H_
