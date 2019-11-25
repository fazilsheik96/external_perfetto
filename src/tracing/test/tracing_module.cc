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

#include "src/tracing/test/tracing_module.h"

#include "protos/perfetto/trace/track_event/log_message.pbzero.h"
#include "src/tracing/test/tracing_module_categories.h"

#include <stdio.h>

// This file is for checking that multiple sets of trace event categories can be
// combined into the same program.

PERFETTO_TRACK_EVENT_STATIC_STORAGE();

namespace tracing_module {

void InitializeCategories() {
  TrackEvent::Register();
}

void EmitTrackEvents() {
  TRACE_EVENT_BEGIN("cat1", "DisabledEventFromModule");
  TRACE_EVENT_END("cat1");
  TRACE_EVENT_BEGIN("cat4", "DisabledEventFromModule");
  TRACE_EVENT_END("cat4");
  TRACE_EVENT_BEGIN("cat9", "DisabledEventFromModule");
  TRACE_EVENT_END("cat9");
  TRACE_EVENT_BEGIN("foo", "FooEventFromModule");
  TRACE_EVENT_END("foo");
}

perfetto::internal::TrackEventIncrementalState* GetIncrementalState() {
  perfetto::internal::TrackEventIncrementalState* state = nullptr;
  TrackEvent::Trace([&state](TrackEvent::TraceContext ctx) {
    state = ctx.GetIncrementalState();
  });
  return state;
}

void FunctionWithOneTrackEvent() {
  TRACE_EVENT_BEGIN("cat1", "DisabledEventFromModule");
  // Simulates the non-tracing work of this function, which should take priority
  // over the above trace event in terms of instruction scheduling.
  puts("Hello");
}

void FunctionWithOneTrackEventWithTypedArgument() {
  TRACE_EVENT_BEGIN("cat1", "EventWithArg", [](perfetto::EventContext ctx) {
    auto log = ctx.event()->set_log_message();
    log->set_body_iid(0x42);
  });
  // Simulates the non-tracing work of this function, which should take priority
  // over the above trace event in terms of instruction scheduling.
  puts("Hello");
}

void FunctionWithOneScopedTrackEvent() {
  TRACE_EVENT("cat1", "ScopedEventFromModule");
  // Simulates the non-tracing work of this function, which should take priority
  // over the above trace event in terms of instruction scheduling.
  puts("Hello");
}

}  // namespace tracing_module
