/*
 * Copyright (C) 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "src/trace_processor/db/row_map.h"

namespace perfetto {
namespace trace_processor {

namespace {

RowMap SelectRangeWithRange(uint32_t start,
                            uint32_t end,
                            uint32_t selector_start,
                            uint32_t selector_end) {
  PERFETTO_DCHECK(start <= end);
  PERFETTO_DCHECK(selector_start <= selector_end);
  PERFETTO_DCHECK(selector_end <= end - start);

  return RowMap(start + selector_start, start + selector_end);
}

RowMap SelectRangeWithBv(uint32_t start,
                         uint32_t end,
                         const BitVector& selector) {
  PERFETTO_DCHECK(start <= end);

  BitVector bv(start, false);
  bv.Resize(end, true);
  bv.UpdateSetBits(selector);
  return RowMap(std::move(bv));
}

RowMap SelectRangeWithIv(uint32_t start,
                         uint32_t end,
                         const std::vector<uint32_t>& selector) {
  PERFETTO_DCHECK(start <= end);

  std::vector<uint32_t> iv(selector.size());
  for (uint32_t i = 0; i < selector.size(); ++i) {
    PERFETTO_DCHECK(selector[i] < end - start);
    iv[i] = selector[i] + start;
  }
  return RowMap(std::move(iv));
}

RowMap SelectBvWithRange(const BitVector& bv,
                         uint32_t selector_start,
                         uint32_t selector_end) {
  PERFETTO_DCHECK(selector_start <= selector_end);
  PERFETTO_DCHECK(selector_end <= bv.GetNumBitsSet());

  BitVector ret = bv.Copy();
  for (auto it = ret.IterateSetBits(); it; it.Next()) {
    auto set_idx = it.set_bit_index();
    if (set_idx < selector_start || set_idx >= selector_end)
      it.Clear();
  }
  return RowMap(std::move(ret));
}

RowMap SelectBvWithBv(const BitVector& bv, const BitVector& selector) {
  BitVector ret = bv.Copy();
  ret.UpdateSetBits(selector);
  return RowMap(std::move(ret));
}

RowMap SelectBvWithIv(const BitVector& bv,
                      const std::vector<uint32_t>& selector) {
  std::vector<uint32_t> iv(selector.size());
  for (uint32_t i = 0; i < selector.size(); ++i) {
    // TODO(lalitm): this is pretty inefficient.
    iv[i] = bv.IndexOfNthSet(selector[i]);
  }
  return RowMap(std::move(iv));
}

RowMap SelectIvWithRange(const std::vector<uint32_t>& iv,
                         uint32_t selector_start,
                         uint32_t selector_end) {
  PERFETTO_DCHECK(selector_start <= selector_end);
  PERFETTO_DCHECK(selector_end <= iv.size());

  std::vector<uint32_t> ret(selector_end - selector_start);
  for (uint32_t i = selector_start; i < selector_end; ++i) {
    ret[i - selector_start] = iv[i];
  }
  return RowMap(std::move(ret));
}

RowMap SelectIvWithBv(const std::vector<uint32_t>& iv,
                      const BitVector& selector) {
  RowMap rm(iv);
  uint32_t idx = 0;
  rm.RemoveIf([&idx, &selector](uint32_t) { return !selector.IsSet(idx++); });
  return rm;
}

RowMap SelectIvWithIv(const std::vector<uint32_t>& iv,
                      const std::vector<uint32_t>& selector) {
  std::vector<uint32_t> ret(selector.size());
  for (uint32_t i = 0; i < selector.size(); ++i) {
    PERFETTO_DCHECK(selector[i] < iv.size());
    ret[i] = iv[selector[i]];
  }
  return RowMap(std::move(ret));
}

}  // namespace

RowMap::RowMap() : RowMap(0, 0) {}

RowMap::RowMap(uint32_t start, uint32_t end)
    : mode_(Mode::kRange), start_idx_(start), end_idx_(end) {}

RowMap::RowMap(BitVector bit_vector)
    : mode_(Mode::kBitVector), bit_vector_(std::move(bit_vector)) {}

RowMap::RowMap(std::vector<uint32_t> vec)
    : mode_(Mode::kIndexVector), index_vector_(std::move(vec)) {}

RowMap RowMap::Copy() const {
  switch (mode_) {
    case Mode::kRange:
      return RowMap(start_idx_, end_idx_);
    case Mode::kBitVector:
      return RowMap(bit_vector_.Copy());
    case Mode::kIndexVector:
      return RowMap(index_vector_);
  }
  PERFETTO_FATAL("For GCC");
}

RowMap RowMap::SelectRowsSlow(const RowMap& selector) const {
  // Pick the strategy based on the selector as there is more common code
  // between selectors of the same mode than between the RowMaps being
  // selected of the same mode.
  switch (selector.mode_) {
    case Mode::kRange:
      switch (mode_) {
        case Mode::kRange:
          return SelectRangeWithRange(start_idx_, end_idx_, selector.start_idx_,
                                      selector.end_idx_);
        case Mode::kBitVector:
          return SelectBvWithRange(bit_vector_, selector.start_idx_,
                                   selector.end_idx_);
        case Mode::kIndexVector:
          return SelectIvWithRange(index_vector_, selector.start_idx_,
                                   selector.end_idx_);
      }
      break;
    case Mode::kBitVector:
      switch (mode_) {
        case Mode::kRange:
          return SelectRangeWithBv(start_idx_, end_idx_, selector.bit_vector_);
        case Mode::kBitVector:
          return SelectBvWithBv(bit_vector_, selector.bit_vector_);
        case Mode::kIndexVector:
          return SelectIvWithBv(index_vector_, selector.bit_vector_);
      }
      break;
    case Mode::kIndexVector:
      switch (mode_) {
        case Mode::kRange:
          return SelectRangeWithIv(start_idx_, end_idx_,
                                   selector.index_vector_);
        case Mode::kBitVector:
          return SelectBvWithIv(bit_vector_, selector.index_vector_);
        case Mode::kIndexVector:
          return SelectIvWithIv(index_vector_, selector.index_vector_);
      }
      break;
  }
  PERFETTO_FATAL("For GCC");
}

}  // namespace trace_processor
}  // namespace perfetto