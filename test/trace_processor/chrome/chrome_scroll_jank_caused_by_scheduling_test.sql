--
-- Copyright 2022 The Android Open Source Project
--
-- Licensed under the Apache License, Version 2.0 (the "License");
-- you may not use this file except in compliance with the License.
-- You may obtain a copy of the License at
--
--     https://www.apache.org/licenses/LICENSE-2.0
--
-- Unless required by applicable law or agreed to in writing, software
-- distributed under the License is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
-- See the License for the specific language governing permissions and
-- limitations under the License.

SELECT RUN_METRIC('chrome/chrome_scroll_jank_caused_by_scheduling.sql',
  'dur_causes_jank_ms',
/* dur_causes_jank_ms = */ '5');

SELECT
  full_name,
  total_duration_ms,
  total_thread_duration_ms,
  count,
  window_start_ts,
  window_end_ts,
  scroll_type
FROM chrome_scroll_jank_caused_by_scheduling;
